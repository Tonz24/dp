#version 450

layout(location = 0) out vec2 outNDCxy;

#include "common.glsl"

vec2 positions[6] = vec2[](
    vec2(-1.0f, 1.0f),
    vec2(1.0f,-1.0f),
    vec2(1.0f,1.0f),

    vec2(-1.0f, 1.0f),
    vec2(-1.0f,-1.0f),
    vec2(1.0f,-1.0f)
);

void main() {
    outNDCxy = positions[gl_VertexIndex];
    gl_Position = vec4(outNDCxy,0,1);
}