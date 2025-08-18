#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoord;

layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform sampler2D diffAlbedoMap;
layout(set = 1, binding = 1) uniform sampler2D specAlbedoMap;
layout(set = 1, binding = 2) uniform sampler2D normalMap;
layout(set = 1, binding = 3) uniform sampler2D shininessMap;


layout(push_constant) uniform PushConstants {
    mat4 matM;
    mat4 matN;
    uint matIndex;
} pcs;

#include "common.glsl"

void main() {
    Material mat = materialUBO.materials[pcs.matIndex];

    vec3 diffColor = mat.diffuseAlbedo;
    diffColor = texture(shininessMap, texCoord).xyz;


    outColor = vec4(diffColor, 1.0);
}