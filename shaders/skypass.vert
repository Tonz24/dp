#version 450

layout(location = 0) out vec2 outTexCoord;

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
    outTexCoord = positions[gl_VertexIndex];
    gl_Position = vec4(outTexCoord,0,1);
}