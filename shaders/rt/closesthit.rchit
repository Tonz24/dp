//https://github.com/yknishidate/single-file-vulkan-pathtracing
//https://nvpro-samples.github.io/vk_raytracing_tutorial_KHR/

#version 460
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_ray_tracing : require

#define CLOSEST_HIT_DIFFUSE

#include "../common/common.glsl"
#include "../common/math_constants.glsl"
#include "pcs/pcs_raygen.glsl"
#include "ris.glsl"

layout(location = 0) rayPayloadInEXT HitPayload payload;

hitAttributeEXT vec2 attribs;

void main() {
    vec3 posWS;
    vec3 emission;
    ShadeParams params = resolveHit(gl_InstanceCustomIndexEXT, gl_PrimitiveID, attribs, gl_WorldRayDirectionEXT, gl_ObjectToWorldEXT, posWS, emission);

    // passed from raygen shader, put into a separate variable, otherwise there's VK_DEVICE_LOST if used directly as an inout parameter
    uint seed = payload.seed;

    vec4 nextSample = sampleHemisphereCosineWeighted(params.normal,seed);
    vec3 nextDir = nextSample.xyz;
    float pdf = nextSample.w;

    payload.hitPosition = posWS;
    payload.hitEmission = emission;
    vec3 hitBrdf = params.albedo * INVPI;
    payload.hit = true;
    payload.nextSample = nextSample;
    payload.hitNormal =  params.normal;

    payload.weightFactor = hitBrdf * max(dot(nextDir,params.normal),0.0) / pdf;

    if(!hitLight(payload)){
         payload.directContribution = doRIS()
            ? calculateDirectRIS(params,posWS,gl_WorldRayDirectionEXT,MAT_DIFFUSE,seed)  
            : calculateDirect(params,posWS,gl_WorldRayDirectionEXT,MAT_DIFFUSE,seed);
    }

    payload.seed = seed;
    payload.mirror = false;
}

#undef CLOSEST_HIT_DIFFUSE
