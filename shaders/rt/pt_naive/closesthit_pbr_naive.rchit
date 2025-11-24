//https://github.com/yknishidate/single-file-vulkan-pathtracing
//https://nvpro-samples.github.io/vk_raytracing_tutorial_KHR/

#version 460
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_ray_tracing : require

#include "../../common/common.glsl"
#include "../../common/math_constants.glsl"
#include "../pcs/pcs_raygen.glsl"
#include "../raycommon.glsl"
#include "../structs/payload.glsl"

layout(location = 0) rayPayloadInEXT HitPayloadNaive payload;

hitAttributeEXT vec2 attribs;

void main() {
    vec3 posWS;
    vec3 emission;
    ShadeParams params = resolveHit(gl_InstanceCustomIndexEXT, gl_PrimitiveID, attribs, gl_WorldRayDirectionEXT, gl_ObjectToWorldEXT, posWS, emission);

    // passed from raygen shader, put into a separate variable, otherwise there's VK_DEVICE_LOST if used directly as an inout parameter
    uint seed = payload.seed;

    vec3 brdf = vec3(0.0);
    vec4 nextSample = samplePbr(gl_WorldRayDirectionEXT,params,seed,brdf);

    float cos_theta_i = max(dot(nextSample.xyz, params.normal), 0.0f);

    payload.hitPosition = posWS;
    payload.hitEmission = emission;
    payload.hit = true;
    payload.nextSample = nextSample;
    payload.hitNormal =  params.normal;

    if (nextSample.w > 0.0f)
        payload.weightFactor = brdf * cos_theta_i / nextSample.w;

    payload.seed = seed;
}