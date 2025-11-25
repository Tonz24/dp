#ifndef RIS_GLSL
#define RIS_GLSL

#include "mis.glsl"
#include "pcs/pcs_raygen.glsl"

uint M_brdf;
uint M_area;
uint M_env;

struct CandidateSample{
	vec3 omega_i;
    vec3 L_i;
	float W;
	float misWeight;
};


CandidateSample makeEmptyCandidate(){
    CandidateSample candidate;

    candidate.omega_i = vec3(0.0);
    candidate.L_i = vec3(0.0);
    candidate.W = 0.0;
    candidate.misWeight = 0.0f;

    return candidate;
}

struct Reservoir{
    CandidateSample bestSample;
    float wSum;
    float W;
};

Reservoir makeEmptyReservoir(){
    Reservoir r;
    r.bestSample = makeEmptyCandidate();
    r.wSum = 0.0;
    r.W = 0.0;

    return r;
}

bool addSample(inout Reservoir reservoir, CandidateSample candidate, float w, inout uint seed){
    reservoir.wSum += w;

    // do not divide by zero
    if (reservoir.wSum <= 0.0)
        return false;

    if (rand(seed) < w / reservoir.wSum){
        reservoir.bestSample = candidate;
        return true;
    }
    return false;
}

float balanceHeuristicArea(float pdfArea, float pdfBrdf){
    float denom = pdfArea * M_area + pdfBrdf * M_brdf;
    return sanitize(pdfArea / denom);
}

float balanceHeursticEnv(float pdfEnv, float pdfBrdf){
    float denom = pdfEnv * M_env + pdfBrdf * M_brdf;
    return sanitize(pdfEnv / denom);
}

float balanceHeuristicBrdf(float pdfBrdf, float pdfArea, float pdfEnv){
    float denom = pdfBrdf * M_brdf + pdfArea * M_area + pdfEnv * M_env;
    return sanitize(pdfBrdf / denom);
}

// samples an area light (emissive triangle) and returns incoming radiance
// area-brdf MIS weighing. environmental map is not taken into consideration, since its the sampling domain is mutually exclusive with the area light domain: 
//      if an environment sample has non-zero incoming radiance at an shading point, it must not be occluded by any scene geometry, which includes emissive surfaces
//      equivalently, if the shading point has non-zero radiance coming from an emissive surface, there is zero radiance coming from that specific direction from the env map due to occlusion   
CandidateSample areaSampleLight(ShadeParams shadeParams, vec3 posWS, vec3 rayDir, uint matType, inout uint seed){
    // take CDF sample (pick an emissive triangle with probability proportional to the area of all emissive triangles)
    uint cdfSampleIndex = sampleCDFIndex(rand(seed));

    if (cdfSampleIndex > emissiveCDF.size)
        return makeEmptyCandidate();

    CDFElement cdfSample = emissiveCDF.cdf[cdfSampleIndex];

    TrianglePacked cdfTriangle = emissiveBuffer.tris[cdfSample.triIndex];
    vec3 L_i = vec3(cdfTriangle.v0eR.w,cdfTriangle.v1eG.w,cdfTriangle.v2eB.w);

    // sample the triangle identified by CDF sample (pick a random point on the triangle with uniform probability)
    TriangleSample sampledPoint = sampleTriangle(cdfTriangle,seed);

    // early exit if occlusion test fails as there wouldn't be any light contribution anyway
    if (!isVisible(posWS, sampledPoint.position, shadeParams.normal)) 
        return makeEmptyCandidate();

    // (1 / triangleArea) * (triangleArea / totalArea) -- triangleArea cancels out
    float lightPdf = 1.0 / emissiveCDF.area;

    vec3 omega_i = sampledPoint.position - posWS;
    float r_sqr = dot(omega_i, omega_i);
    omega_i = normalize(omega_i);

    // early exit when invalid numbers are present
    if (r_sqr == 0.0 || any(isnan(omega_i)) || any(isinf(omega_i)) || lightPdf <= 0.0) 
        return makeEmptyCandidate();

    float cos_theta_i = dot(omega_i, shadeParams.normal);
    float cos_theta_y = abs(dot(-omega_i, sampledPoint.normal));

    // any of these not passing means that radiance is also zero
    if (cos_theta_i <= 0.0f || cos_theta_y == 0.0f)
        return makeEmptyCandidate();

    float solidAngleFactor = r_sqr / cos_theta_y;

    // get the pdf of this sample as if it came from BRDF sampling
    #ifdef CLOSEST_HIT_DIFFUSE
    float brdfPdf = getPdfHemisphereCosineWeighted(omega_i, shadeParams.normal);
    #endif

    #ifdef CLOSEST_HIT_PBR
    float brdfPdf = getPdfPbr(rayDir, omega_i, shadeParams);
    #endif

    #ifdef RAYGEN
    float brdfPdf = getPdfMaterial(rayDir,omega_i, shadeParams, matType);
    #endif

    lightPdf *= solidAngleFactor;

    CandidateSample risSample;

    risSample.omega_i = omega_i;
    risSample.L_i = L_i;
    risSample.W = 1.0f / lightPdf;
    risSample.misWeight = balanceHeuristicArea(lightPdf, brdfPdf);

    return risSample;
}

// TODO: retrieve the texure value immediately after sampling to get rid of double cartesian -> polar -> uv transformation
// samples the environment map and evaluates incoming radiance
// env-brdf MIS weighing. Area lights (emissive triangles) are not taken into consideration since their sampling domain is not overlapping with the env map domain  
CandidateSample envSampleLight(ShadeParams shadeParams, vec3 posWS, vec3 rayDir, uint matType, inout uint seed){
    vec4 envSample = sampleEnvironment(textures[pcs.skyCdfHandle], seed);
    vec3 omega_i = envSample.xyz;
    float envPdf = envSample.w;

    if (envPdf <= 0 || isinf(envPdf) || isnan(envPdf))
        return makeEmptyCandidate();

    //  if the ray hits anything (env map is occluded), early exit
    //  occlusion of the env map means that zero radiance would come from this direction anyway
    if (!isVisible(posWS, omega_i * 1000.f, shadeParams.normal)) 
        return makeEmptyCandidate();

    // retrieve radiance coming from the env map
    vec3 L_i = sampleSphericalMap(omega_i, pcs.skyHandle);

    // get the pdf of this sample as if it came from BRDF sampling
    #ifdef CLOSEST_HIT_DIFFUSE
    float brdfPdf = getPdfHemisphereCosineWeighted(omega_i, shadeParams.normal);
    #endif

    #ifdef CLOSEST_HIT_PBR
    float brdfPdf = getPdfPbr(rayDir, omega_i, shadeParams);
    #endif

    #ifdef RAYGEN
    float brdfPdf = getPdfMaterial(rayDir,omega_i, shadeParams, matType);
    #endif

    CandidateSample risSample;

    risSample.omega_i = omega_i;
    risSample.L_i = L_i;
    risSample.W = 1.0f / envPdf;
    risSample.misWeight = balanceHeursticEnv(envPdf, brdfPdf);

    return risSample;
}

CandidateSample brdfSampleLight(ShadeParams shadeParams, vec3 posWS, vec3 rayDir, uint matType, inout uint seed){

    // sample the BRDF
    #ifdef CLOSEST_HIT_DIFFUSE
    vec4 brdfSample = sampleHemisphereCosineWeighted(shadeParams.normal, seed);
    #endif

    #ifdef CLOSEST_HIT_PBR
    vec4 brdfSample = samplePbr(rayDir,shadeParams,seed);
    #endif

    #ifdef RAYGEN
    vec4 brdfSample = sampleMaterial(rayDir, shadeParams, matType, seed);
    #endif
    
    vec3 omega_i = brdfSample.xyz;
    float brdfPdf = brdfSample.w;

    if (isnan(brdfPdf) || isinf(brdfPdf) || brdfPdf <= 0.0f)
        return makeEmptyCandidate();

    // check what the BRDF sample hits 
    TracedSample brdfHit = traceBRDFLighting(posWS, omega_i, shadeParams.normal);
    vec3 L_i = brdfHit.hitEmission;

    // if the sample can't contribute any radiance (it hit a non-emissive scene surface) do an early exit
    // TODO: reuse the sample for next bounce direction
    if (brdfHit.didHit && !hitLight(brdfHit.hitEmission))
        return makeEmptyCandidate();

    float areaPdf = 0.0;
    float envPdf = 0.0;

    // emissive surface case -- calculate the pdf of hitting the emissive surface, convert it to solid angle measure
    if (brdfHit.didHit){
        L_i = brdfHit.hitEmission;

        vec3 toHit = brdfHit.hitPosition - posWS;
        float r_sqr = dot(toHit, toHit);
        float cos_theta_y = abs(dot(-omega_i, brdfHit.hitNormal));

        if (r_sqr <= 0.0 || cos_theta_y <= 0.0)
            return makeEmptyCandidate();

        float solidAngleFactor =  r_sqr / cos_theta_y;
        areaPdf = (1.0 / emissiveCDF.area) * solidAngleFactor;
    }
    // evaluating the env pdf only makes sense when the sample is unoccluded by scene geometry
    else {
        envPdf = getPdfEnvironment(textures[pcs.skyCdfHandle],omega_i);
    }
   
    CandidateSample risSample;

    risSample.omega_i = omega_i;
    risSample.L_i = L_i;
    risSample.W = 1.0f / brdfPdf;
    risSample.misWeight = balanceHeuristicBrdf(brdfPdf, areaPdf, envPdf);

    return risSample;
}

vec3 evalF(CandidateSample candidate, ShadeParams shadeParams, vec3 posWS, vec3 rayDir, uint matType){
    // evaluate brdf
    #ifdef CLOSEST_HIT_DIFFUSE
    vec3 brdf = evalBrdfDiffuse(shadeParams.albedo);
    #endif

    #ifdef CLOSEST_HIT_PBR
    vec3 brdf = evalBrdfPbr(rayDir, candidate.omega_i, shadeParams);
    #endif

    #ifdef RAYGEN
    vec3 brdf = evalBrdfMaterial(rayDir, candidate.omega_i, shadeParams, matType);
    #endif

    float cos_theta_i = max(dot(candidate.omega_i, shadeParams.normal),0.0f);

    vec3 L_direct = brdf * candidate.L_i * cos_theta_i;
    return L_direct;
}

float evalPhat(CandidateSample candidate, ShadeParams shadeParams, vec3 posWS, vec3 rayDir, uint matType){
    return length(evalF(candidate, shadeParams, posWS, rayDir, matType));
}

float evalPhat(CandidateSample candidate, ShadeParams shadeParams, vec3 posWS, vec3 rayDir, uint matType, inout vec3 F){
    F = evalF(candidate, shadeParams, posWS, rayDir, matType);
    return length(F);
}

vec3 calculateDirectRIS(ShadeParams shadeParams, vec3 posWS, vec3 rayDir, uint matType, inout uint seed){

    Reservoir r = makeEmptyReservoir();

    M_brdf = getBrdfSampleCount(); 
    M_area = getAreaSampleCount(); 
    M_env = getEnvSampleCount(); 

    for (int i = 0; i < M_brdf; ++i) {
		CandidateSample candidate = brdfSampleLight(shadeParams, posWS, rayDir, matType, seed);

		float p_hat = evalPhat(candidate, shadeParams, posWS, rayDir, matType);
        float w = candidate.misWeight * p_hat * candidate.W;
        addSample(r, candidate, w, seed);
	}

    for (int i = 0; i < M_area; ++i) {
        CandidateSample candidate = areaSampleLight(shadeParams, posWS, rayDir, matType, seed);

        float p_hat = evalPhat(candidate, shadeParams, posWS, rayDir, matType);
        float w = candidate.misWeight * p_hat * candidate.W;
        addSample(r, candidate, w, seed);
    }

    for (int i = 0; i < M_env; ++i) {
		CandidateSample candidate = envSampleLight(shadeParams, posWS, rayDir, matType, seed);

		float p_hat = evalPhat(candidate, shadeParams, posWS, rayDir, matType);
        float w = candidate.misWeight * p_hat * candidate.W;
        addSample(r, candidate, w, seed);
	}

    if (r.wSum <= 0.0f)
        return vec3(0.0);

    vec3 F = vec3(0.0);
    float p_hat = evalPhat(r.bestSample, shadeParams, posWS, rayDir, matType, F);
    r.W = p_hat > 0.0f ? 1.0f / p_hat * r.wSum : 0.0f;

    vec3 directContribution = F * r.W;

    return directContribution;
}

#endif // RIS_GLSL
