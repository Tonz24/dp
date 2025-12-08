#ifndef CLOSEST_HIT_BASE_GLSL
#define CLOSEST_HIT_BASE_GLSL

//https://github.com/yknishidate/single-file-vulkan-pathtracing
//https://nvpro-samples.github.io/vk_raytracing_tutorial_KHR/
#include "../common/common.glsl"
#include "../common/math_constants.glsl"
#include "pcs/pcs_raygen.glsl"
#include "payload.glsl"
#include "raycommon.glsl"
#include "../common/material.glsl"

#ifdef EVAL_DIRECT_CONTRIB
#include "ris.glsl"
#endif

#ifdef PAYLOAD_NAIVE
layout(location = 0) rayPayloadInEXT HitPayloadNaive payload;
#else
layout(location = 0) rayPayloadInEXT HitPayload payload;
#endif


hitAttributeEXT vec2 attribs;

void main() {
    // resolve hit -- get all attributes needed for evaluating the hit
    vec3 posWS;
    vec3 emission;
    ShadeParams params = resolveHit(gl_InstanceCustomIndexEXT, gl_PrimitiveID, attribs, gl_WorldRayDirectionEXT, gl_ObjectToWorldEXT, posWS, emission);

    // passed from raygen shader, put into a separate variable, otherwise there's VK_DEVICE_LOST if used directly as an inout parameter
    uint seed = payload.seed;

    // prefill payload attributes that are always the same regardless of the hit material
    payload.hitPosition = posWS;
    payload.hitEmission = emission;
    payload.hitNormal =  params.normal;
    payload.hit = true;

    #ifndef PAYLOAD_NAIVE
    payload.mirror = false;
    #endif

    // sample the material and evaluate its brdf
    // nextSample.xyz -- sample direction (omega_i)
    // nextSample.w -- pdf of the sample
    vec3 brdf;

    #ifdef CLOSEST_HIT_DIFFUSE
    vec4 nextSample = sampleHemisphereCosineWeighted(params, seed, brdf);
    #endif

    #ifdef CLOSEST_HIT_MIRROR
    vec4 nextSample = sampleMirror(gl_WorldRayDirectionEXT, params, brdf);

    #ifndef PAYLOAD_NAIVE
    payload.mirror = true;
    #endif

    #endif

    #ifdef CLOSEST_HIT_PBR
    vec4 nextSample = samplePbr(gl_WorldRayDirectionEXT, params, seed, brdf);
    #endif

    // calculate the cosine factor
    float cos_theta_i = max(dot(nextSample.xyz,params.normal),0.0);

    //set the sample and evaluate weight
    payload.nextSample = nextSample;
    payload.weightFactor = sanitize(brdf * cos_theta_i / nextSample.w);

    // evaluate direct contribution if its defined in the closest hit shader
    #ifdef EVAL_DIRECT_CONTRIB
    if(!hitLight(payload)){
         payload.directContribution = doRIS()
            ? calculateDirectRIS(params, posWS, gl_WorldRayDirectionEXT, 0, seed)  
            : calculateDirect(params, posWS, gl_WorldRayDirectionEXT, 0, seed);
    }
    #endif

    // pass manipulated seed back to payload (and thus the raygen shader)
    payload.seed = seed;
}



#endif // CLOSEST_HIT_BASE_GLSL
