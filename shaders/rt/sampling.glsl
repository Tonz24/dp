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


// https://auzaiffe.wordpress.com/2024/04/15/vndf-importance-sampling-an-isotropic-distribution/
// https://gist.github.com/jdupuy/4c6e782b62c92b9cb3d13fbb0a5bd7a0#file-samplevndf_ggx-cpp-L51
vec3 SampleVndf_GGX(vec2 u, vec3 wi, float alpha, vec3 n) {
    
    // Dirac function for alpha = 0
    if (alpha == 0.0f) return n;
    // decompose the vector in parallel and perpendicular components
    vec3 wi_z = n * dot(wi, n);
    vec3 wi_xy = wi - wi_z;
    // warp to the hemisphere configuration
    vec3 wiStd = normalize(wi_z - alpha * wi_xy);
    // sample a spherical cap in (-wiStd.z, 1]
    float wiStd_z = dot(wiStd, n);
    float phi = (2.0f * u.x - 1.0f) * PI;
    float z = (1.0f - u.y) * (1.0f + wiStd_z) - wiStd_z;
    float sinTheta = sqrt(clamp(1.0f - z * z, 0.0f, 1.0f));
    float x = sinTheta * cos(phi);
    float y = sinTheta * sin(phi);
    vec3 cStd = vec3(x, y, z);
    // reflect sample to align with normal
    vec3 up = vec3(0, 0, 1);
    vec3 wr = n + up;
    // prevent division by zero
    float wrz_safe = max(wr.z, 1e-6);
    vec3 c = dot(wr, cStd) * wr / wrz_safe - cStd;
    // compute halfway direction as standard normal
    vec3 wmStd = c + wiStd;
    vec3 wmStd_z = n * dot(n, wmStd);
    vec3 wmStd_xy = wmStd_z - wmStd;
    // warp back to the ellipsoid configuration
    vec3 wm = normalize(wmStd_z + alpha * wmStd_xy);
    // return final normal
    //wm = toYUp * wm;
    return wm;
}


// https://auzaiffe.wordpress.com/2024/04/15/vndf-importance-sampling-an-isotropic-distribution/
float pdf_vndf_isotropic(vec3 wo, vec3 wi, float alpha, vec3 n) {

    float alphaSquare = alpha * alpha;
    vec3 wm = normalize(wo + wi);
    float zm = dot(wm, n);
    float zi = dot(wi, n);
    float nrm = inversesqrt((zi * zi) * (1.0f - alphaSquare) + alphaSquare);
    float sigmaStd = (zi * nrm) * 0.5f + 0.5f;
    float sigmaI = sigmaStd / nrm;
    float nrmN = (zm * zm) * (alphaSquare - 1.0f) + 1.0f;
    return alphaSquare / (PI * 4.0f * nrmN * nrmN * sigmaI);
}
/*
vec4 samplePbr(vec3 normal, vec3 rayDir, float alpha, inout uint seed){
	vec3 omega_o = -gl_WorldRayDirectionEXT;

	float ksi1 = rand(seed);
    float ksi2 = rand(seed);

    vec3 omega_h = SampleVndf_GGX(vec2(ksi1,ksi2), omega_o, alpha, params.normal); // microfacet normal, halfway vector between omega_o (viewDir) and omega_i (next bounce dir for specular)




}*/


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