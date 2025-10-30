//https://github.com/yknishidate/single-file-vulkan-pathtracing
//https://nvpro-samples.github.io/vk_raytracing_tutorial_KHR/

#version 460
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_ray_tracing : require

#include "../common/common.glsl"
#include "../common/math_constants.glsl"
#include "pcs/pcs_raygen.glsl"
#include "raycommon.glsl"
#include "structs/payload.glsl"

layout(location = 0) rayPayloadInEXT HitPayload payload;
layout(location = 1) rayPayloadEXT BRDFSamplePayload payloadBRDF;

hitAttributeEXT vec2 attribs;


TracedSample traceBRDFLighting(vec3 posWS, vec3 direction){
    float tMin = 0.001f;
    float tMax = 10000.0f;

    resetBRDFSamplePayload(payloadBRDF);
    traceRayEXT(
            topLevelAS,
            gl_RayFlagsOpaqueEXT,
            0xff, // cullMask
            2,    // sbtRecordOffset
            0,    // sbtRecordStride
            0,    // missIndex
            posWS,
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

vec3 evaluateSampleAreaMisBrdf(vec3 albedo, vec3 posWS, vec3 normal, inout uint seed){
    // take CDF sample (pick an emissive triangle with probability proportional to the area of all emissive triangles)
    uint cdfSampleIndex = sampleCDFIndex(rand(seed));

    if (cdfSampleIndex > emissiveCDF.size)
        return vec3(0.0);

    CDFElement cdfSample = emissiveCDF.cdf[cdfSampleIndex];

    TrianglePacked cdfTriangle = emissiveBuffer.tris[cdfSample.triIndex];
    vec3 L_i = vec3(cdfTriangle.v0eR.w,cdfTriangle.v1eG.w,cdfTriangle.v2eB.w);

    // sample the triangle identified by CDF sample (pick a random point on the triangle with uniform probability)
    TriangleSample sampledPoint = sampleTriangle(cdfTriangle,seed);

    // early exit if occlusion test fails as there wouldn't be any light contribution anyway
    if (!isVisible(posWS, sampledPoint.position, normal)) return vec3(0.0);

    float pdfCDF = cdfSampleIndex == 0 ? cdfSample.cdfVal : cdfSample.cdfVal - emissiveCDF.cdf[cdfSampleIndex-1].cdfVal;
    float lightPdf = sampledPoint.pdf * pdfCDF;

    vec3 omega_i = sampledPoint.position - posWS;
    float r_sqr = dot(omega_i, omega_i);
    omega_i = normalize(omega_i);

    // early exit when invalid numbers are present
    if (r_sqr == 0.0 || any(isnan(omega_i)) || any(isinf(omega_i)) || lightPdf == 0.0) return vec3(0.0);

    float cos_theta_i = max(dot(omega_i, normal),0.0);
    float cos_theta_y = abs(dot(-omega_i, sampledPoint.normal));

    // any of these being zero means that radiance is also zero
    if (cos_theta_i == 0.0 || cos_theta_y == 0.0) return vec3(0.0);

    float areaMeasureFactor = cos_theta_y / r_sqr;
    // get the pdf of this sample as if it came from BRDF sampling
    float brdfPdf = getPdfHemisphereCosineWeighted(omega_i,normal);
    // convert to area measure
    brdfPdf *= areaMeasureFactor;

    float misWeight = powerHeuristic(lightPdf, brdfPdf);
    if (misWeight <= 0.0) return vec3(0.0);

    vec3 brdf = albedo / PI;
    float G = areaMeasureFactor;
    vec3 L_direct = misWeight * L_i * brdf * G * cos_theta_i / lightPdf;

    return L_direct;
}

// TODO: retrieve the texure value immediately after sampling to get rid of double cartesian -> polar -> uv transformation
// samples the environment map and evaluates incoming radiance
// env-brdf MIS weighing. Area lights (emissive triangles) are not taken into consideration since their sampling domain is not overlapping with the env map domain  
vec3 evaluateSampleEnvMisBrdf(vec3 albedo, vec3 posWS, vec3 normal, inout uint seed){
    vec4 envSample = sampleEnvironment(textures[pcs.skyCdfHandle], seed);
    vec3 omega_i = envSample.xyz;
    float envPdf = envSample.w;

    //  send a ray in the env map sample direction
    TracedSample brdfHit = traceBRDFLighting(posWS, omega_i);

    //  if the ray hits anything (env map is occluded), early exit
    //  occlusion of the env map means that zero radiance would come from this direction anyway
    if (brdfHit.didHit) return vec3(0.0);

    // retrieve radiance coming from the env map
    vec3 L_i = sampleSphericalMap(omega_i, pcs.skyHandle);

    // get the pdf of the env map sample as if it came from sampling the BRDF
    float brdfPdf = getPdfHemisphereCosineWeighted(omega_i, normal);

    float misWeight = powerHeuristic(envPdf, brdfPdf);

    if (misWeight <= 0.0) return vec3(0.0);

    float cos_theta_i = max(dot(omega_i, normal), 0.0f);

    vec3 brdf = albedo * INVPI;
    vec3 L_direct = misWeight * L_i * brdf * cos_theta_i / envPdf; 
    return L_direct;

}

vec3 evaluateSampleBrdfMisEnvArea(vec3 albedo, vec3 posWS, vec3 normal, inout uint seed){

    // sample the BRDF
    vec4 brdfSample = sampleHemisphereCosineWeighted(normal, seed);
    vec3 omega_i = brdfSample.xyz;
    float brdfPdf = brdfSample.w;

    // check what the BRDF sample hits 
    TracedSample brdfHit = traceBRDFLighting(posWS, omega_i);
    vec3 L_i = brdfHit.hitEmission;

    // if the sample can't contribute any radiance (it hit a non-emissive scene surface) do an early exit
    // TODO: reuse the sample for next bounce direction
    if (brdfHit.didHit && all(lessThanEqual(brdfHit.hitEmission,vec3(0.0))))
        return vec3(0.0);

    float pdfOther = 0;

    // emissive surface case -- calculate the pdf of hitting the emissive surface, convert it to solid angle measure
    if (brdfHit.didHit){
        L_i = brdfHit.hitEmission;

        vec3 toHit = brdfHit.hitPosition - posWS;
        float r_sqr = dot(toHit, toHit);
        float cos_theta_y = abs(dot(-omega_i, brdfHit.hitNormal));

        if (r_sqr <= 0.0 || cos_theta_y <= 0.0) return vec3(0.0);

        float areaToSolidMeasureFactor =  r_sqr / cos_theta_y;
        pdfOther = (1 / emissiveCDF.area) * areaToSolidMeasureFactor;
    }
    // evaluating the env pdf only makes sense when the sample is unoccluded by scene geometry
    else {
        pdfOther = getPdfEnvironment(textures[pcs.skyCdfHandle],omega_i);
        if (pdfOther <= 0.0) return vec3(0.0);
    }
    
    float misWeight = powerHeuristic(brdfPdf, pdfOther);
    float cos_theta_i = max(dot(omega_i, normal), 0.0f);

    vec3 brdf = albedo * INVPI;
    vec3 L_direct = misWeight * L_i * brdf * cos_theta_i / brdfPdf; 
    return L_direct;
}

vec3 calculateDirect(vec3 albedo, vec3 posWS, vec3 normal, inout uint seed){
    vec3 directContribution = vec3(0.0);

    directContribution += evaluateSampleAreaMisBrdf(albedo, posWS, normal, seed);
    directContribution += evaluateSampleEnvMisBrdf(albedo, posWS, normal, seed);
    directContribution += evaluateSampleBrdfMisEnvArea(albedo, posWS, normal, seed);

    return directContribution;
}

void main() {
    ObjDesc object = objDesc.i[gl_InstanceCustomIndexEXT];

    Material material = materialUBO.materials[object.materialId];
    Indices indices = Indices(object.indexBufferAddr);
    Vertices vertices = Vertices(object.vertexBufferAddr);

    ivec3 ind = indices.i[gl_PrimitiveID];
    Vertex v0 = vertices.v[ind.x];
    Vertex v1 = vertices.v[ind.y];
    Vertex v2 = vertices.v[ind.z];

    mat4x3 modelMat = gl_ObjectToWorldEXT;
    mat3 normalMat = transpose(inverse(mat3(gl_ObjectToWorldEXT)));

    vec3 uv = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);
    vec2 texCoord = getTexCoord(uv, v0, v1, v2);
    vec3 hitNormal = getNormalWS(normalMat, uv, v0, v1, v2);
    vec3 posWS = getPositionWS(modelMat, uv, v0, v1, v2);
    vec3 hitTangent = getTangentWS(normalMat, uv, v0, v1, v2);

    if (dot(-gl_WorldRayDirectionEXT,hitNormal) < 0.0)
        hitNormal *= -1.0;

    vec3 T = hitTangent - dot(hitTangent, hitNormal) * hitNormal;
    T = normalize(T);
    vec3 B = normalize(cross(hitNormal, T));
    mat3 TBN = mat3(T, B, hitNormal);

    ShadeParams params = unpackMaterial(material, hitNormal, TBN, texCoord);


    // flip normal if backside is hit
    if (dot(-gl_WorldRayDirectionEXT,params.normal) < 0.0)
        params.normal *= -1.0;

    // passed from raygen shader, put into a separate variable, otherwise there's VK_DEVICE_LOST if used directly as an inout parameter
    uint seed = payload.seed;

    vec4 nextSample = sampleHemisphereCosineWeighted(params.normal,seed);
    vec3 nextDir = nextSample.xyz;
    float pdf = nextSample.w;

    payload.hitPosition = posWS;
    payload.hitEmission = material.emission;
    vec3 hitBrdf = params.albedo * INVPI;
    payload.hit = true;
    payload.nextSample = nextSample;
    payload.hitNormal =  params.normal;

    payload.weightFactor = hitBrdf * max(dot(nextDir,params.normal),0.0) / pdf;

    if(!hitLight(payload))
        payload.directContribution = calculateDirect(params.albedo,posWS,params.normal,seed);

    payload.seed = seed;
}