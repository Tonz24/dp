#version 460

layout(location = 0) in vec3 inNormal;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in mat3 inTBN;

layout(location = 0) out vec4 outAlbedo;
layout(location = 1) out vec4 outNormal;
layout(location = 2) out uint outMeshId;
layout(location = 3) out uint outMaterialId;


#include "common.glsl"
#include "pcs_gbuffer_fill.glsl"

void main() {
    Material mat = materialUBO.materials[pcs.matIndex];

    float hasAlbedoMap = clamp(float(mat.diffuseAlbedoMapHandle),0.0f,1.0f);
    vec3 albedo = mix(mat.diffuseAlbedo, texture(textures[mat.diffuseAlbedoMapHandle], inTexCoord).rgb, hasAlbedoMap);


    float hasNormalMap = clamp(float(mat.normalMapHandle),0.0f,1.0f);
    vec3 normal = mix(normalize(inNormal),normalize(inTBN * (texture(textures[mat.normalMapHandle],inTexCoord).xyz * 2.0 - 1.0)),hasNormalMap);

    float hasShininessMap = clamp(float(mat.shininessMapHandle),0.0f,1.0f);
    float shininess = mix(mat.shininess, texture(textures[mat.shininessMapHandle], inTexCoord).r, hasShininessMap);

    // roughness to shininess remapping https://simonstechblog.blogspot.com/2011/12/microfacet-brdf.html
    shininess = mix(shininess,2.0f / (shininess * shininess) - 2.0f,hasShininessMap);

    // smuggle tex coords for bindless test
    outAlbedo = vec4(albedo, inTexCoord.x);
    outNormal = vec4(normal,shininess);
    outMeshId = pcs.meshId;
    outMaterialId = pcs.matIndex;
}