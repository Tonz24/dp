#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_GOOGLE_include_directive : enable

#include "raycommon.glsl"
#include "structs/payload.glsl"

layout(location = 0) rayPayloadInEXT HitPayload payload;

void main() {
    resetPayload(payload);
}
