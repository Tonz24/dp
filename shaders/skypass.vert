#version 450

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inTexCoord;


layout(location = 0) out vec2 outTexCoord;

#include "common.glsl"

void main() {
    //gl_Position = vec4(inPosition,0,1);
    gl_Position = vec4(inTexCoord,0,1);
    outTexCoord = inTexCoord;
}