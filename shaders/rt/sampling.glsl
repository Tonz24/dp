#include "../common/rng.glsl"
#include "pbr.glsl"

vec3 orthogonal(vec3 vec) {
	return abs(vec.x) > abs(vec.z) ? vec3(vec.y, -vec.x, 0.0f ) : vec3(0.0f, vec.z, -vec.y);
}

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


vec3 evalBrdfDiffuse(vec3 albedo){
	return albedo * INVPI;
}

vec3 evalBrdfMirror(vec3 albedo){
	return albedo;
}

/*
vec4 evalBrdfMaterial(vec3 normal, vec3 rayDir, uint materialType, inout uint seed){
	vec4 sample = vec4(0.0);

	// diffuse
	if (materialType == 0)
		return sampleHemisphereCosineWeighted(normal, seed);
	if (materialType == 1)
		return sampleMirror(normal, rayDir);

	return sample;
}*/





vec4 samplePbr(vec3 normal, vec3 rayDir, float alpha, inout uint seed){
	vec3 omega_o = -gl_WorldRayDirectionEXT;

	float ksi1 = rand(seed);
    float ksi2 = rand(seed);

    vec3 omega_h = SampleVndf_GGX(vec2(ksi1,ksi2), omega_o, alpha, params.normal); // microfacet normal, halfway vector between omega_o (viewDir) and omega_i (next bounce dir for specular)




}


vec4 sampleMaterial(vec3 normal, vec3 rayDir, uint materialType, inout uint seed){
	vec4 sampled = vec4(0.0);

	// diffuse
	if (materialType == 0)
		return sampleHemisphereCosineWeighted(normal, seed);
	// mirror
	if (materialType == 1)
		return sampleMirror(normal, rayDir);

	// pbr test
	if (materialType == 2){
		

	}

	return sampled;
}


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