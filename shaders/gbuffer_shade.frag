#version 450

layout(location = 0) in vec2 inNDCxy;

layout(location = 0) out vec4 fragColor;

layout(set = 1, binding = 0) uniform sampler2D albedoMap;
layout(set = 1, binding = 1) uniform sampler2D normalMap;
layout(set = 1, binding = 2) uniform sampler2D depthMap;
layout(set = 1, binding = 3) uniform usampler2D materialMap;

#include "common.glsl"
#include "pcs_gbuffer_shade.glsl"




void main() {
    vec2 texCoord = inNDCxy * 0.5 + 0.5;

    vec3 albedo = texture(albedoMap,texCoord).xyz;
    vec3 normal = texture(normalMap,texCoord).xyz;

    bool hasValidGeometry = (bool((normal.x > 0 || normal.x < 0) || (normal.y > 0 || normal.y < 0) || (normal.z > 0 || normal.z < 0)));

    if (hasValidGeometry)
        fragColor = vec4(albedo,1.0);
    else
        fragColor = vec4(1.0);
}