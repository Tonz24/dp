//https://github.com/yknishidate/single-file-vulkan-pathtracing
//https://nvpro-samples.github.io/vk_raytracing_tutorial_KHR/

#version 460
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_ray_tracing : require

#include "../../common/common.glsl"
#include "../../common/math_constants.glsl"
#include "../raycommon.glsl"
#include "../structs/payload.glsl"
#include "../pcs/pcs_raygen.glsl"

layout(location = 0) rayPayloadInEXT HitPayloadNaive payload;
hitAttributeEXT vec2 attribs;


void main() {
     vec3 posWS;
    vec3 emission;
    ShadeParams params = resolveHit(gl_InstanceCustomIndexEXT, gl_PrimitiveID, attribs, gl_WorldRayDirectionEXT, gl_ObjectToWorldEXT, posWS, emission);

    // passed from raygen shader, put into a separate variable, otherwise there's VK_DEVICE_LOST if used directly as an inout parameter
    uint seed = payload.seed;

    vec4 nextSample = sampleMirror(params.normal,gl_WorldRayDirectionEXT);

    payload.hitPosition = posWS;
    payload.hitEmission = emission;
    vec3 hitBrdf = params.albedo;
    payload.hit = true;
    payload.nextSample = nextSample;

    payload.weightFactor = hitBrdf;
}