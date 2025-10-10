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