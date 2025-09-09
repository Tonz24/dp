#version 450

layout(location = 0) in vec3 inNormal;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in mat3 inTBN;

layout(location = 0) out vec4 outAlbedo;
layout(location = 1) out vec4 outNormal;
layout(location = 2) out uint outMeshId;
layout(location = 3) out uint outMaterialId;


layout(set = 1, binding = 0) uniform sampler2D diffAlbedoMap;
layout(set = 1, binding = 1) uniform sampler2D specAlbedoMap;
layout(set = 1, binding = 2) uniform sampler2D normalMap;
layout(set = 1, binding = 3) uniform sampler2D shininessMap;


#include "common.glsl"
#include "pcs_gbuffer_fill.glsl"

void main() {
    Material mat = materialUBO.materials[pcs.matIndex];

    float hasAlbedoMap = clamp(float(mat.diffuseAlbedoMapHandle),0.0f,1.0f);
    vec3 albedo = mix(mat.diffuseAlbedo, texture(textures[mat.diffuseAlbedoMapHandle], inTexCoord).rgb, hasAlbedoMap);


    float hasNormalMap = clamp(float(mat.normalMapHandle),0.0f,1.0f);
    vec3 normal = mix(normalize(inNormal),normalize(inTBN * (texture(textures[mat.normalMapHandle],inTexCoord).xyz * 2.0 - 1.0)),hasNormalMap);

    // smuggle tex coords for bindless test
    outAlbedo = vec4(albedo, inTexCoord.x);
    outNormal = vec4(normal,inTexCoord.y);
    outMeshId = pcs.meshId;
    outMaterialId = pcs.matIndex;
}