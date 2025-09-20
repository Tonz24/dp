
// https://github.com/yknishidate/single-file-vulkan-pathtracing/blob/master/shaders/common.glsl
uint pcg(inout uint state) {

    uint prev = state * 747796405u + 2891336453u;
    uint word = ((prev >> ((prev >> 28u) + 4u)) ^ prev) * 277803737u;
    state = prev;
    return (word >> 22u) ^ word;
}

// https://github.com/yknishidate/single-file-vulkan-pathtracing/blob/master/shaders/common.glsl
uvec2 pcg2d(uvec2 v) {

    v = v * 1664525u + 1013904223u;
    v.x += v.y * 1664525u;
    v.y += v.x * 1664525u;
    v = v ^ (v >> 16u);
    v.x += v.y * 1664525u;
    v.y += v.x * 1664525u;
    v = v ^ (v >> 16u);
    return v;
}

float rand(inout uint seed) {

    uint val = pcg(seed);
    return (float(val) * (1.0 / float(0xffffffffu)));
}

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