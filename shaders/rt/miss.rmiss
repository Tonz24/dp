//https://github.com/yknishidate/single-file-vulkan-pathtracing
//https://nvpro-samples.github.io/vk_raytracing_tutorial_KHR/
#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_GOOGLE_include_directive : enable

#include "../common/common.glsl"
#include "raycommon.glsl"
#include "structs/payload.glsl"
#include "pcs/pcs_raygen.glsl"

layout(location = 0) rayPayloadInEXT HitPayload payload;

void main() {
    resetPayload(payload);
    payload.hitEmission = pcs.sampleSky == 1 ? sampleSphericalMap(gl_WorldRayDirectionEXT, pcs.skyHandle) : vec3(0.0);
    payload.hit = false;
}
