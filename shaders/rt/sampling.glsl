#include "../common/rng.glsl"

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

vec4 sampleMaterial(vec3 normal, vec3 rayDir, uint materialType, inout uint seed){
	vec4 sampled = vec4(0.0);

	// diffuse
	if (materialType == 0)
		return sampleHemisphereCosineWeighted(normal, seed);
	// mirror
	if (materialType == 1)
		return sampleMirror(normal, rayDir);

	return sampled;
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

uint sampleEnvCdfMarginalIndex(readonly image2D envCdf, float rnd){
    uint left = 0;

    uint right = uint(imageSize(envCdf).y);

    while (left < right){
        uint mid = left + (right - left) / 2;

		float val = imageLoad(envCdf, ivec2(0,mid)).r;

        if (rnd < val)
            right = mid;
        else
            left = mid + 1;
    }
    return left;
}

uint sampleEnvCdfConditionalIndex(readonly image2D envCdf, uint y, float rnd){
    uint left = 0;
    uint right = uint(imageSize(envCdf).x);

    while (left < right){
        uint mid = left + (right - left) / 2;

		float val = imageLoad(envCdf, ivec2(mid,y)).r;

        if (rnd < val)
            right = mid;
        else
            left = mid + 1;
    }
    return left;
}

vec4 sampleEnvironment(readonly image2D envCdf, inout uint seed){

	vec2 envSize = vec2(imageSize(envCdf));
	float r1 = rand(seed);
	float r2 = rand(seed);

	// sample pixel
	uint y = sampleEnvCdfMarginalIndex(envCdf, r1);
	uint x = sampleEnvCdfConditionalIndex(envCdf, y, r2);

	// calculate texel pdf using the cdf
	float pdfY = y > 0 ? imageLoad(envCdf, ivec2(0, y)).r - imageLoad(envCdf, ivec2(0, y - 1)).r : imageLoad(envCdf, ivec2(0,y)).r;
	float pdfX = x > 0 ? imageLoad(envCdf, ivec2(x, y)).r - imageLoad(envCdf, ivec2(x - 1, y)).r : imageLoad(envCdf, ivec2(x, y)).r;

	float pdf = pdfX * pdfY;

	// sample within the pixel using r1 and r2 as interpolants
	float vInterpA = y > 0 ? imageLoad(envCdf, ivec2(0, y - 1)).r : 0.0f;
	float vInterpB = imageLoad(envCdf, ivec2(0, y)).r;
	float vInterp = (r1 - vInterpA) / (vInterpB - vInterpA);

	float uInterpA = x > 0.0f ? imageLoad(envCdf, ivec2(x - 1, y)).r : 0.0f;
	float uInterpB = imageLoad(envCdf, ivec2(x, y)).r;
	float uInterp = (r2 - uInterpA) / (uInterpB - uInterpA);

	// add the interpolated offset to sampled pixel coordinates, convert to [0, 1] range
	float u = (float(x) + uInterp) / envSize.x;
	float v = (float(y) + vInterp) / envSize.y;

	// convert uv to spherical coordinates
	float theta = v * PI;
	float phi = u * TWOPI;

	float sinTheta = sin(theta);
	float cosTheta = cos(theta);
	float sinPhi = sin(phi);
	float cosPhi = cos(phi);

	vec3 direction = vec3(sinTheta * cosPhi, sinTheta * sinPhi, cosTheta);
	pdf = pdf / ((2.0 * PI / envSize.x) * (PI / envSize.y) * sinTheta);

	return vec4(direction, pdf);
}