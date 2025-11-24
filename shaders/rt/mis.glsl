#ifndef MIS_GLSL
#define MIS_GLSL

#include "structs/payload.glsl"
#include "raycommon.glsl"
#include "../common/material.glsl"

#ifndef PAYLOAD_BRDF
#define PAYLOAD_BRDF

layout(location = 1) rayPayloadEXT BRDFSamplePayload payloadBRDF;

#endif // PAYLOAD_BRDF

// trace a ray for BRDF lighting
TracedSample traceBRDFLighting(vec3 posWS, vec3 direction, vec3 normal){
    float tMin = 0.001f;
    float tMax = 10000.0f;

    resetBRDFSamplePayload(payloadBRDF);
    traceRayEXT(
            topLevelAS,
            gl_RayFlagsOpaqueEXT,
            0xff, // cullMask
            3,    // start at brdf sample region
            0,    // sbtRecordStride
            1,    // missIndex
            posWS + normal * tMin,
            tMin,
            direction,
            tMax,
            1     // brdf sample always has payload location 1
    );
    
    return makeTraced(payloadBRDF);
}

float powerHeuristic(float pdf, float pdfOther) {
	const float pdf_sqr = pdf * pdf;
	const float pdfOther_sqr = pdfOther * pdfOther;
	const float result = pdf_sqr / (pdfOther_sqr + pdf_sqr);
	return result;
}

// samples an area light (emissive triangle) and returns incoming radiance
// area-brdf MIS weighing. environmental map is not taken into consideration, since its the sampling domain is mutually exclusive with the area light domain: 
//      if an environment sample has non-zero incoming radiance at an shading point, it must not be occluded by any scene geometry, which includes emissive surfaces
//      equivalently, if the shading point has non-zero radiance coming from an emissive surface, there is zero radiance coming from that specific direction from the env map due to occlusion   
vec3 evaluateSampleAreaMisBrdf(ShadeParams shadeParams, vec3 posWS, vec3 rayDir, uint matType, inout uint seed){
    // take CDF sample (pick an emissive triangle with probability proportional to the area of all emissive triangles)
    uint cdfSampleIndex = sampleCDFIndex(rand(seed));

    if (cdfSampleIndex > emissiveCDF.size)
        return vec3(0.0);

    CDFElement cdfSample = emissiveCDF.cdf[cdfSampleIndex];

    TrianglePacked cdfTriangle = emissiveBuffer.tris[cdfSample.triIndex];
    vec3 L_i = vec3(cdfTriangle.v0eR.w,cdfTriangle.v1eG.w,cdfTriangle.v2eB.w);

    // sample the triangle identified by CDF sample (pick a random point on the triangle with uniform probability)
    TriangleSample sampledPoint = sampleTriangle(cdfTriangle,seed);

    float visibility = float(isVisible(posWS, sampledPoint.position, shadeParams.normal));

    // (1 / triangleArea) * (triangleArea / totalArea) -- triangleArea cancels out
    float lightPdf = 1.0 / emissiveCDF.area;

    vec3 omega_i = sampledPoint.position - posWS;
    float r_sqr = dot(omega_i, omega_i);
    omega_i = normalize(omega_i);

    float cos_theta_i = max(dot(omega_i, shadeParams.normal), 0.0);
    float cos_theta_y = abs(dot(-omega_i, sampledPoint.normal));

    float areaMeasureFactor = cos_theta_y / r_sqr;

    #ifdef CLOSEST_HIT_DIFFUSE
    // get the pdf of this sample as if it came from BRDF sampling
    float brdfPdf = getPdfHemisphereCosineWeighted(omega_i, shadeParams.normal);
    #endif

    #ifdef CLOSEST_HIT_PBR
    // get the pdf of this sample as if it came from BRDF sampling
    float brdfPdf = getPdfPbr(rayDir, omega_i, shadeParams);
    #endif

    #ifdef RAYGEN
    // get the pdf of this sample as if it came from BRDF sampling
    float brdfPdf = getPdfMaterial(rayDir,omega_i, shadeParams, matType);
    #endif

    // convert to area measure
    brdfPdf *= areaMeasureFactor;

    float misWeight = powerHeuristic(lightPdf, brdfPdf);

    // evaluate brdf
    #ifdef CLOSEST_HIT_DIFFUSE
    vec3 brdf = evalBrdfDiffuse(shadeParams.albedo);
    #endif

    #ifdef CLOSEST_HIT_PBR
    vec3 brdf = evalBrdfPbr(rayDir, omega_i, shadeParams);
    #endif

    #ifdef RAYGEN
    vec3 brdf = evalBrdfMaterial(rayDir,omega_i, shadeParams, matType);
    #endif

    float G = areaMeasureFactor;
    vec3 L_direct = misWeight * visibility * L_i * brdf * G * cos_theta_i / lightPdf;

    return L_direct;
}

// TODO: retrieve the texure value immediately after sampling to get rid of double cartesian -> polar -> uv transformation
// samples the environment map and evaluates incoming radiance
// env-brdf MIS weighing. Area lights (emissive triangles) are not taken into consideration since their sampling domain is not overlapping with the env map domain  
vec3 evaluateSampleEnvMisBrdf(ShadeParams shadeParams, vec3 posWS, vec3 rayDir, uint matType, inout uint seed){
    vec4 envSample = sampleEnvironment(textures[pcs.skyCdfHandle], seed);
    vec3 omega_i = envSample.xyz;
    float envPdf = envSample.w;

    float visibility = float(isVisible(posWS, omega_i * 1000.f, shadeParams.normal));

    // retrieve radiance coming from the env map
    vec3 L_i = sampleSphericalMap(omega_i, pcs.skyHandle);

    #ifdef CLOSEST_HIT_DIFFUSE
    // get the pdf of this sample as if it came from BRDF sampling
    float brdfPdf = getPdfHemisphereCosineWeighted(omega_i, shadeParams.normal);
    #endif

    #ifdef CLOSEST_HIT_PBR
    // get the pdf of this sample as if it came from BRDF sampling
    float brdfPdf = getPdfPbr(rayDir, omega_i, shadeParams);
    #endif

    #ifdef RAYGEN
    // get the pdf of this sample as if it came from BRDF sampling
    float brdfPdf = getPdfMaterial(rayDir,omega_i, shadeParams, matType);
    #endif

    float misWeight = powerHeuristic(envPdf, brdfPdf);

    float cos_theta_i = max(dot(omega_i, shadeParams.normal), 0.0f);

    // evaluate brdf
    #ifdef CLOSEST_HIT_DIFFUSE
    vec3 brdf = evalBrdfDiffuse(shadeParams.albedo);
    #endif

    #ifdef CLOSEST_HIT_PBR
    vec3 brdf = evalBrdfPbr(rayDir, omega_i, shadeParams);
    #endif

    #ifdef RAYGEN
    vec3 brdf = evalBrdfMaterial(rayDir,omega_i, shadeParams, matType);
    #endif

    vec3 L_direct = misWeight * visibility * L_i * brdf * cos_theta_i / envPdf; 
    return L_direct;
}

vec3 evaluateSampleBrdfMisEnvArea(ShadeParams shadeParams, vec3 posWS, vec3 rayDir, uint matType, inout uint seed){
    vec3 brdfValue = vec3(0.0);

    // sample the BRDF
    #ifdef CLOSEST_HIT_DIFFUSE
    vec4 brdfSample = sampleHemisphereCosineWeighted(shadeParams.normal, seed);
    brdfValue = evalBrdfDiffuse(shadeParams.albedo);
    #endif

    #ifdef CLOSEST_HIT_PBR
    vec4 brdfSample = samplePbr(rayDir,shadeParams,seed,brdfValue);
    #endif

    #ifdef RAYGEN
    vec4 brdfSample = sampleMaterial(rayDir, shadeParams, matType, seed, brdfValue);
    #endif
    
    vec3 omega_i = brdfSample.xyz;
    float brdfPdf = brdfSample.w;

    bool unfolded = false;
    // check what the BRDF sample hits 
    TracedSample brdfHit = traceBRDFLighting(posWS, omega_i, shadeParams.normal);
    vec3 L_i = brdfHit.hitEmission;

    // if the sample can't contribute any radiance (it hit a non-emissive scene surface) do an early exit
    // TODO: reuse the sample for next bounce direction
    if (brdfHit.didHit && all(lessThanEqual(brdfHit.hitEmission,vec3(0.0))))
        return vec3(0.0);

    float pdfOther = 0.0;

    // emissive surface case -- calculate the pdf of hitting the emissive surface, convert it to solid angle measure
    if (brdfHit.didHit){
        L_i = brdfHit.hitEmission;

        vec3 toHit = brdfHit.hitPosition - posWS;
        float r_sqr = dot(toHit, toHit);
        float cos_theta_y = abs(dot(-omega_i, brdfHit.hitNormal));

        float areaToSolidMeasureFactor =  r_sqr / cos_theta_y;
        pdfOther = (1.0 / emissiveCDF.area) * areaToSolidMeasureFactor;
    }
    // evaluating the env pdf only makes sense when the sample is unoccluded by scene geometry
    else
        pdfOther = getPdfEnvironment(textures[pcs.skyCdfHandle],omega_i);

    float misWeight = powerHeuristic(brdfPdf, pdfOther);

    float cos_theta_i = max(dot(omega_i, shadeParams.normal), 0.0f);

    vec3 L_direct = misWeight * L_i * brdfValue * cos_theta_i / brdfPdf; 
    return L_direct;
}

vec3 calculateDirect(ShadeParams shadeParams, vec3 posWS, vec3 rayDir, uint matType, inout uint seed){
    vec3 directContribution = vec3(0.0);

    vec3 contribArea = evaluateSampleAreaMisBrdf(shadeParams, posWS, rayDir, matType, seed);
    vec3 contribEnv = evaluateSampleEnvMisBrdf(shadeParams, posWS, rayDir, matType, seed);
    vec3 contribBrdf = evaluateSampleBrdfMisEnvArea(shadeParams, posWS, rayDir, matType, seed);

    directContribution = contribArea + contribEnv + contribBrdf;
    float validSample = float(!(any(isinf(directContribution)) || any(isnan(directContribution)) || any(lessThan(directContribution,vec3(0.0)))));

    return directContribution + (1.0 - validSample) * vec3(0,10,0);
}

#endif //MIS_GLSL
