#version 450

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inTexCoord;


layout(location = 0) out vec2 outTexCoord;

#include "common.glsl"


vec2 positions[4] = vec2[](
    vec2(1.0f,1.0f),
    vec2(1.0f,-1.0f),
    vec2(-1.0f,-1.0f),
    vec2(-1.0f, 1.0f)
);

vec2 positions2[6] = vec2[](
    vec2(-1.0f, 1.0f),
    vec2(1.0f,-1.0f),
    vec2(1.0f,1.0f),

    vec2(-1.0f, 1.0f),
    vec2(-1.0f,-1.0f),
    vec2(1.0f,-1.0f)
);

void main() {
    outTexCoord = positions2[gl_VertexIndex];
    gl_Position = vec4(outTexCoord,0,1);
}