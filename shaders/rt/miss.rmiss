#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_GOOGLE_include_directive : enable

#include "raycommon.glsl"

layout(location = 0) rayPayloadInEXT HitPayload payload;

void main() {
    payload.hitValue = vec3(0,0,0);
}
