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

void main() {
    gl_Position = vec4(inPosition,0,1);
    //outTexCoord = inPosition;
    outTexCoord = positions[gl_VertexIndex];

    //outTexCoord = vec2(gl_VertexIndex, gl_VertexIndex) / 4.0;
}