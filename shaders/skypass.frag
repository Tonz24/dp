#version 450

layout(location = 0) in vec2 texCoord;

layout(location = 0) out vec4 fragColor;

layout(set = 1, binding = 0) uniform sampler2D skyTexture;

#include "common.glsl"

#define PI 3.1415926f
#define TWOPI PI*2.0
#define INVPI 1.0/PI
#define INV2PI 1.0/(TWOPI)

vec2 dirToUv(vec3 dir){
    const float u = 0.5f + 0.5f * atan(dir.z, dir.x) * INVPI;
    const float v = 1.0f - acos(dir.y) * INVPI;
    return vec2(u,v);
}

vec3 sampleSphericalMap(vec3 dir, sampler2D sphericalTex){
    vec2 uv = dirToUv(dir);
    return texture(sphericalTex,uv).xyz;
}

vec3 aces(vec3 x) {
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

void main() {
    vec4 ndc = vec4(texCoord,-1,1);
    vec4 camRay = vec4(cameraUBO.matInvVP * vec4(ndc.xy,1,1));
    vec3 dir = normalize(camRay.xyz / camRay.w - cameraUBO.posWS);

    vec3 envMapColor = sampleSphericalMap(dir, skyTexture);
    envMapColor = aces(envMapColor);

    fragColor = vec4(envMapColor,1.0);
}