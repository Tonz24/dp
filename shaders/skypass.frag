#version 450

layout(location = 0) in vec2 inNDCxy;

layout(location = 0) out vec4 fragColor;

layout(set = 1, binding = 0) uniform sampler2D skyTexture;

#include "common.glsl"
#include "tonemappers.glsl"
#include "constants.glsl"

vec2 dirToUv(vec3 dir){
    const float u = 0.5f + 0.5f * atan(dir.z, dir.x) * INVPI;
    const float v = 1.0f - acos(dir.y) * INVPI;
    return vec2(u,v);
}

vec3 sampleSphericalMap(vec3 dir, sampler2D sphericalTex){
    vec2 uv = dirToUv(dir);
    return texture(sphericalTex,uv).xyz;
}

void main() {
    vec4 camRay = cameraUBO.matInvVP * vec4(inNDCxy,1,1);
    vec3 dir = normalize(camRay.xyz / camRay.w - cameraUBO.posWS);

    vec3 envMapColor = sampleSphericalMap(dir, skyTexture);
    envMapColor = aces(envMapColor);

    fragColor = vec4(envMapColor,1.0);
}