#version 460
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_ray_tracing : require

#define CLOSEST_HIT_DIFFUSE
#define EVAL_DIRECT_CONTRIB

#include "closesthit_base.glsl"
