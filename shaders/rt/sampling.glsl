#ifndef SAMPLING_GLSL
#define SAMPLING_GLSL

#include "../common/rng.glsl"
#include "../common/material.glsl"
#include "pbr.glsl"

vec3 orthogonal(vec3 vec) {
	return abs(vec.x) > abs(vec.z) ? vec3(vec.y, -vec.x, 0.0f ) : vec3(0.0f, vec.z, -vec.y);
}

//====================BRDF SAMPLING====================
vec4 sampleHemisphereCosineWeighted(vec3 normal, inout uint seed){

	float r1 = rand(seed);
	float r2 = rand(seed);

	float x = cos(PI * 2.0f * r1) * sqrt(1.0f - r2);
	float y = sin(PI * 2.0f * r1) * sqrt(1.0f - r2);
	float z = sqrt(r2);

	// local reference frame
	vec3 sampled = normalize(vec3(x,y,z));

	vec3 o2 = normalize(orthogonal(normal));
	vec3 o1 = normalize(cross(normal, o2));
	o2 = normalize(cross(o1, normal));

	mat3 toWorld = mat3(o1,o2,normal);

	//transform to world space
	sampled = toWorld * sampled;

	float pdf = max(dot(normal, sampled), 0.0f) * INVPI;

	return vec4(sampled, pdf);
}

float getPdfHemisphereCosineWeighted(vec3 sampled, vec3 normal){
	return max(dot(normal, sampled), 0.0f) * INVPI;
}

vec4 sampleMirror(vec3 normal, vec3 rayDir){
	vec3 sampled = reflect(rayDir,normal);
	return vec4(sampled, 1.0);
}

vec4 samplePbr(vec3 rayDir, ShadeParams shadeParams, inout uint seed){

	float alpha = shadeParams.roughness * shadeParams.roughness;

	vec3 omega_o = -rayDir;

	float ksi1 = rand(seed);
    float ksi2 = rand(seed);

	// evaluate Fresnel term first and use it to choose between a diffuse or specular sample
	float cos_theta_o = max(dot(shadeParams.normal, omega_o), 0.0f); // angle between microfacet normal and view direction

	vec3 F0 = mix(vec3(0.04), shadeParams.albedo, shadeParams.metallic);
    vec3 F = fresnelSchlick(F0, cos_theta_o);
    float FMax = max(F.x,max(F.y,F.z));

    float specProb = FMax;

	float ksi3 = rand(seed);
    bool isSpecularBounce = ksi3 < specProb;

	float pdf_spec = 0.0;
    float pdf_diff = 0.0;

	vec4 nextSample = vec4(0.0);
	if (isSpecularBounce) {
        // evaluate omega_i (bounce direction): reflect view direction (omega_o) around the microfacet normal (omega_h)
		// microfacet normal, halfway vector between omega_o (viewDir) and omega_i (next bounce dir for specular)
		vec3 omega_h = normalize(SampleVndf_GGX(vec2(ksi1,ksi2), omega_o, alpha, shadeParams.normal)); 

        vec3 omega_i = reflect(-omega_o, omega_h);
        nextSample.xyz = omega_i;
		// figure out what the pdf of the omega_i would be if it came from cosine weighted sampling
        pdf_diff = getPdfHemisphereCosineWeighted(omega_i, shadeParams.normal);
    } 
    else {
		// sample the hemisphere using cosine weighted sampling to get the diffuse sample
        vec4 sampled = sampleHemisphereCosineWeighted(shadeParams.normal, seed);
        nextSample.xyz = sampled.xyz; 

        pdf_diff = sampled.w; 
    }

	pdf_spec = pdf_vndf_isotropic(nextSample.xyz , omega_o, alpha, shadeParams.normal);

	// mixture pdfs
	nextSample.w = pdf_diff * (1.0 - specProb) + pdf_spec * specProb;
	return nextSample;
}

float getPdfPbr(vec3 rayDir, vec3 omega_i, ShadeParams shadeParams){
	float alpha = shadeParams.roughness * shadeParams.roughness;

	vec3 omega_o = -rayDir;

	float cos_theta_o = max(dot(shadeParams.normal, omega_o), 0.0f); // angle between microfacet normal and view direction

	vec3 F0 = mix(vec3(0.04), shadeParams.albedo, shadeParams.metallic);
    vec3 F = fresnelSchlick(F0, cos_theta_o);
    float FMax = max(F.x,max(F.y,F.z));

    float specProb = FMax;

	float pdf_diff = getPdfHemisphereCosineWeighted(omega_i, shadeParams.normal);
	float pdf_spec = pdf_vndf_isotropic(omega_i, omega_o, alpha, shadeParams.normal);

	float pdf = pdf_diff * (1.0 - specProb) + pdf_spec * specProb;
	return pdf;
}


vec4 sampleMaterial(vec3 rayDir, ShadeParams shadeParams, uint matType, inout uint seed){
	vec4 sampled = vec4(0.0);

	if (matType == MAT_DIFFUSE)
		return sampleHemisphereCosineWeighted(shadeParams.normal, seed);
	if (matType == MAT_MIRROR)
		return sampleMirror(shadeParams.normal, rayDir);
	if (matType == MAT_PBR)
		return samplePbr(rayDir,shadeParams,seed);

	return sampled;
}


float getPdfMaterial(vec3 rayDir, vec3 omega_i, ShadeParams shadeParams, uint matType){
	float pdf = 0.0f;

	if (matType == MAT_DIFFUSE)
		return getPdfHemisphereCosineWeighted(omega_i, shadeParams.normal);
	if (matType == MAT_MIRROR)
		return 1.0f;
	if (matType == MAT_PBR)
		return getPdfPbr(rayDir, omega_i, shadeParams);

	return pdf;
}
//=====================================================



//======================BRDF EVAL======================

vec3 evalBrdfDiffuse(vec3 albedo){
	return albedo * INVPI;
}

vec3 evalBrdfMirror(vec3 albedo){
	return albedo;
}

vec3 evalBrdfPbr(vec3 rayDir, vec3 lightDir, ShadeParams shadeParams){

	vec3 omega_o = -rayDir;
	vec3 omega_i = lightDir;

	float cos_theta_i = dot(omega_i, shadeParams.normal);

	if (cos_theta_i <= 0.0)
		return vec3(0.0);

    float alpha = shadeParams.roughness * shadeParams.roughness;

    vec3 omega_h = normalize(omega_o + omega_i); // microfacet normal, halfway vector between omega_o (viewDir) and omega_i (next bounce dir for specular)

    // evaluate Fresnel term first and use it to choose between a diffuse or specular sample
    float cos_theta_h = max(dot(omega_h, omega_o), 0.0f); // angle between microfacet normal and view direction

    vec3 F0 = mix(vec3(0.04), shadeParams.albedo, shadeParams.metallic);
    vec3 F = fresnelSchlick(F0, cos_theta_h);

	float cos_theta_m = max(dot(omega_h, shadeParams.normal), 0.0f); // angle between microfacet normal and surface normal
	float cos_theta_o = max(dot(omega_o, shadeParams.normal), 0.0); // angle between view direction and surface normal

	float NDF = distrGGX(alpha, cos_theta_m);
	float G = G_Smith(cos_theta_o, cos_theta_i, alpha); 

	// specular part (Torrance-Sparrow)
	vec3 brdf = NDF * G * F / (4.0f * cos_theta_o * cos_theta_i);

	// calculate diffuse coefficient (1 - specular)
	vec3 kD = (1.0f - F) * (1.0f - shadeParams.metallic);

	// diffuse part (Lambert)
	brdf += kD * evalBrdfDiffuse(shadeParams.albedo);
	
   	return brdf;
}

vec3 evalBrdfMaterial(vec3 rayDir, vec3 lightDir, ShadeParams shadeParams, uint matType){
	vec3 brdf = vec3(0.0);

	if (matType == MAT_DIFFUSE)
		return evalBrdfDiffuse(shadeParams.albedo);
	if (matType == MAT_MIRROR)
		return evalBrdfMirror(shadeParams.albedo);
	if (matType == MAT_PBR)
		return evalBrdfPbr(rayDir, lightDir, shadeParams);

	return brdf;
}

//=====================================================



//==================SAMPLE + BRDF EVAL=================

vec4 samplePbr(vec3 rayDir, ShadeParams shadeParams, inout uint seed, inout vec3 brdf){

	float alpha = shadeParams.roughness * shadeParams.roughness;
	vec3 omega_o = -rayDir;

	// evaluate Fresnel term first and use it to choose between diffuse or specular sample 

	// angle between view direction and surface normal
	float cos_theta_o = max(dot(omega_o, shadeParams.normal), 0.0); 

	vec3 F0 = mix(vec3(0.04), shadeParams.albedo, shadeParams.metallic);
    vec3 F_view = fresnelSchlick(F0, cos_theta_o);
    float FMax = max(F_view.x, max(F_view.y, F_view.z));

	// sanity check
    float specProb = clamp(FMax, 0.0f, 1.0f);

	// choose between specular or diffuse bounce
	float ksi3 = rand(seed);
    bool isSpecularBounce = ksi3 < specProb;

	// preinitialize pdf variables for mixturing later
	float pdf_spec = 0.0;
    float pdf_diff = 0.0;

	vec4 nextSample = vec4(0.0);
	brdf = vec3(0.0);
	if (isSpecularBounce) {

		// sample GGX VNDF to obtain omega_h -- microfacet normal
		float ksi1 = rand(seed);
		float ksi2 = rand(seed);
		// microfacet normal, halfway vector between omega_o (viewDir) and omega_i (next bounce dir for specular)
		vec3 omega_h = normalize(SampleVndf_GGX(vec2(ksi1,ksi2), omega_o, alpha, shadeParams.normal)); 

        // evaluate omega_i (bounce direction): reflect view direction (omega_o) around the microfacet normal (omega_h)
        nextSample.xyz = reflect(-omega_o, omega_h);

	 	if (dot(nextSample.xyz, shadeParams.normal) <= 0.0)
            return vec4(0.0);

        pdf_diff = getPdfHemisphereCosineWeighted(nextSample.xyz, shadeParams.normal);
        pdf_spec = pdf_vndf_isotropic(nextSample.xyz, omega_o, alpha, shadeParams.normal);
    } 
    else {
        vec4 sampled = sampleHemisphereCosineWeighted(shadeParams.normal, seed);
        nextSample.xyz = sampled.xyz; 

        pdf_diff = sampled.w; 
        pdf_spec = pdf_vndf_isotropic(nextSample.xyz, omega_o, alpha, shadeParams.normal);
    }

	brdf = evalBrdfPbr(rayDir,nextSample.xyz,shadeParams);

	// mixture pdfs
	nextSample.w = pdf_diff * (1.0 - specProb) + pdf_spec * specProb;

	// guard against invalid values, later check only for zeros
	if (isinf(nextSample.w) || isnan(nextSample.w))
		nextSample.w = 0.0f;
		
	return nextSample;
}

vec4 sampleMaterial(vec3 rayDir, ShadeParams shadeParams, uint matType, inout uint seed, inout vec3 brdf){
	vec4 sampled = vec4(0.0);

	if (matType == MAT_DIFFUSE){
		brdf = evalBrdfDiffuse(shadeParams.albedo);
		return sampleHemisphereCosineWeighted(shadeParams.normal, seed);
	}
	if (matType == MAT_MIRROR){
		brdf = evalBrdfMirror(shadeParams.albedo);
		return sampleMirror(shadeParams.normal, rayDir);
	}
	if (matType == MAT_PBR)
		return samplePbr(rayDir,shadeParams,seed, brdf);

	return sampled;
}

//=====================================================


//=====================ENV SAMPLING====================

uint sampleEnvCdfMarginalIndex(sampler2D skyCdf, float rnd){
    uint left = 0;
    uint right = uint(textureSize(skyCdf,0).y);


    while (left < right){
        uint mid = left + (right - left) / 2;

		float val = texelFetch(skyCdf, ivec2(0,mid), 0).r;

        if (rnd < val)
            right = mid;
        else
            left = mid + 1;
    }
    return left;
}

uint sampleEnvCdfConditionalIndex(sampler2D skyCdf, uint y, float rnd){
    uint left = 1;
    uint right = uint(textureSize(skyCdf,0).x);

    while (left < right){
        uint mid = left + (right - left) / 2;

		float val = texelFetch(skyCdf, ivec2(mid,y), 0).r;

        if (rnd < val)
            right = mid;
        else
            left = mid + 1;
    }
    return left;
}

vec4 sampleEnvironment(sampler2D skyCdf, inout uint seed){

	ivec2 envSize = textureSize(skyCdf,0);
    float width = float(envSize.x - 1); // subtract 1 from height (ignore the marginal index)
    float height = float(envSize.y);

	float r1 = rand(seed);
	float r2 = rand(seed);

	// sample pixel
	uint y = sampleEnvCdfMarginalIndex(skyCdf,r1);
	uint x = sampleEnvCdfConditionalIndex(skyCdf,y, r2);
    
	// calculate texel pdf using the cdf
	float pdfY = y == 0 ? texelFetch(skyCdf, ivec2(0, 0), 0).r : texelFetch(skyCdf, ivec2(0, y), 0).r - texelFetch(skyCdf, ivec2(0, y - 1), 0).r;
	float pdfX = x == 1 ? texelFetch(skyCdf, ivec2(1, y), 0).r : texelFetch(skyCdf, ivec2(x, y), 0).r - texelFetch(skyCdf, ivec2(x - 1, y), 0).r;

	float pdf = pdfX * pdfY;

	// sample within the pixel using r1 and r2 as interpolants
	float vInterpA = y > 0 ? texelFetch(skyCdf, ivec2(0, y - 1), 0).r : 0.0f;
	float vInterpB = texelFetch(skyCdf, ivec2(0, y), 0).r;
	float vInterp = (r1 - vInterpA) / (vInterpB - vInterpA);

	float uInterpA = x > 1 ? texelFetch(skyCdf, ivec2(x - 1, y), 0).r : 0.0f;
	float uInterpB = texelFetch(skyCdf, ivec2(x, y), 0).r;
	float uInterp = (r2 - uInterpA) / (uInterpB - uInterpA);

	// add the interpolated offset to sampled pixel coordinates, convert to [0, 1] range
	float u = (float(x - 1) + uInterp) / width;
	float v = (float(y) + vInterp) / height;

	// convert uv to spherical coordinates
	float theta = (1.0 - v) * PI;
	float phi = (u + 0.25) * TWOPI;

	float sinTheta = max(sin(theta), 1e-6);
	float cosTheta = cos(theta);
	float sinPhi = sin(phi);
	float cosPhi = cos(phi);

	// reconstruct XYZ direction from spherical coordinates
	vec3 direction = vec3(sinTheta * cosPhi, cosTheta , sinTheta * sinPhi);

	// convert pdf to solid angle measure
	pdf = pdf / ((TWOPI / width) * (PI / height) * sinTheta);

	return vec4(direction, pdf);
}


float getPdfEnvironment(sampler2D skyCdf, vec3 dir){
    ivec2 envSize = textureSize(skyCdf,0);
	int width = envSize.x - 1; // do not include the marginal index
	int height = envSize.y;

	// convert direction to UV in environment map
	vec2 uv = dirToEquirect(dir);
	float u = fract(uv.x);
    float v = uv.y;

	// calculate unnormalized pixel coordinates
	int x = int(u * float(width)) + 1; // start at index 1 since index 0 is reserved for the marginal map
	int y = int(v * float(height));

	// use cdf to get pdf at pixel coordinates
	float pdfY = y == 0 ? texelFetch(skyCdf, ivec2(0, 0), 0).r : texelFetch(skyCdf, ivec2(0, y), 0).r - texelFetch(skyCdf, ivec2(0, y - 1), 0).r;
	float pdfX = x == 1 ? texelFetch(skyCdf, ivec2(1, y), 0).r : texelFetch(skyCdf, ivec2(x, y), 0).r - texelFetch(skyCdf, ivec2(x - 1, y), 0).r;

	float pdf = pdfX * pdfY;

	// convert uv to spherical coordinates
	float theta = (1.0 - v) * PI;
	float sinTheta = max(sin(theta), 1e-6);
	
	// convert pdf to solid angle measure
	pdf = pdf / ((TWOPI / width) * (PI / height) * sinTheta);
	return pdf;
}

//=====================================================

#endif // SAMPLING_GLSL