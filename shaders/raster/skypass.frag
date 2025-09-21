#version 460
#extension GL_GOOGLE_include_directive : enable

layout(location = 0) in vec2 inNDCxy;

layout(location = 0) out vec4 fragColor;


#include "../common/common.glsl"
#include "../common/tonemappers.glsl"
#include "../common/math_constants.glsl"
#include "pcs/pcs_skypass.glsl"

vec2 dirToUv(vec3 dir){
    const float u = 0.5f + 0.5f * atan(dir.z, dir.x) * INVPI;
    const float v = 1.0f - acos(dir.y) * INVPI;
    return vec2(u,v);
}

vec3 sampleSphericalMap(vec3 dir, sampler2D sphericalTex){
    vec2 uv = dirToUv(dir);
    return texture(textures[pcs.skyIndex],uv).xyz;
}

void main() {
    vec4 camRay = cameraUBO.matInvVP * vec4(inNDCxy,1,1);
    vec3 dir = normalize(camRay.xyz / camRay.w - cameraUBO.posWS);

    vec3 envMapColor = sampleSphericalMap(dir, textures[pcs.skyIndex]);
    envMapColor = aces(envMapColor);

    fragColor = vec4(envMapColor,1.0);
}