#version 450

layout(location = 0) in vec2 inNDCxy;

layout(location = 0) out vec4 fragColor;

layout(set = 1, binding = 0) uniform sampler2D albedoMap;
layout(set = 1, binding = 1) uniform sampler2D normalMap;
layout(set = 1, binding = 2) uniform sampler2D depthMap;
layout(set = 1, binding = 3) uniform sampler2D materialMap;


#include "common.glsl"
#include "pcs_gbuffer_shade.glsl"

void main() {

}