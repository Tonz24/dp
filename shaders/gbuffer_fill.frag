#version 450

layout(location = 0) in vec3 inNormal;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in mat3 inTBN;

layout(location = 0) out vec4 outAlbedo;
layout(location = 1) out vec4 outNormal;
layout(location = 2) out uint outMeshId;


layout(set = 1, binding = 0) uniform sampler2D diffAlbedoMap;
layout(set = 1, binding = 1) uniform sampler2D specAlbedoMap;
layout(set = 1, binding = 2) uniform sampler2D normalMap;
layout(set = 1, binding = 3) uniform sampler2D shininessMap;


#include "common.glsl"

void main() {
    Material mat = materialUBO.materials[pcs.matIndex];

    float hasAlbedoMap = clamp(float(mat.diffuseAlbedoMapHandle),0.0f,1.0f);
    vec3 albedo = mix(mat.diffuseAlbedo, texture(diffAlbedoMap, inTexCoord).rgb, hasAlbedoMap);

    float hasNormalMap = clamp(float(mat.normalMapHandle),0.0f,1.0f);
    vec3 normal = mix(inNormal,normalize(inTBN * (texture(normalMap,inTexCoord).xyz * 2.0 - 1.0)),hasNormalMap);


    outAlbedo = vec4(albedo, 1.0);
    outNormal = vec4(normal,0.0);
    outMeshId = pcs.meshId;
}