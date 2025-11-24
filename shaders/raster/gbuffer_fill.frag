#version 460
#extension GL_GOOGLE_include_directive : require

layout(location = 0) in vec3 inNormal;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in mat3 inTBN;
layout(location = 5) in vec3 inPosWS;

layout(location = 0) out vec4 outAlbedo;
layout(location = 1) out vec4 outNormal;
layout(location = 2) out uint outMeshId;
layout(location = 3) out uint outMaterialId;

#include "../common/common.glsl"
#include "pcs/pcs_gbuffer_fill.glsl"

void main() {
    Material mat = materialUBO.materials[pcs.matIndex];
    ShadeParams params = unpackMaterial(mat, inNormal, inTBN, inTexCoord);
    
    outAlbedo = vec4(params.albedo, params.metallic); // smuggle metallic into albedo texture
    outNormal = vec4(params.normal, params.roughness); // smuggle roughness into normal texture
    outMeshId = pcs.meshId;
    outMaterialId = pcs.matIndex;
}