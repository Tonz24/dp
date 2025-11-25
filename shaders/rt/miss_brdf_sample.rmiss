//https://github.com/yknishidate/single-file-vulkan-pathtracing
//https://nvpro-samples.github.io/vk_raytracing_tutorial_KHR/
#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_GOOGLE_include_directive : enable

#include "../common/common.glsl"
#include "raycommon.glsl"
#include "payload.glsl"
#include "pcs/pcs_raygen.glsl"

layout(location = 1) rayPayloadInEXT BRDFSamplePayload payload;

void main() {
    resetBRDFSamplePayload(payload);
    payload.hitEmission = sampleSphericalMap(gl_WorldRayDirectionEXT, pcs.skyHandle);
    payload.didHit = false;
}
