#version 460
#extension GL_GOOGLE_include_directive : require

layout(location = 0) in vec3 inNormal;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inPosWS;
layout(location = 3) in vec4 inPosCS;
layout(location = 4) in mat3 inTBN;

layout(location = 0) out vec4 outAlbedo;
layout(location = 1) out vec4 outNormal;
layout(location = 2) out vec2 outMotion;
layout(location = 3) out uint outMeshId;
layout(location = 4) out uint outMaterialId;

#include "../common/common.glsl"
#include "../common/rng.glsl"
#include "pcs/pcs_gbuffer_fill.glsl"


vec2 getMotionVector(){
    vec2 motionVec = vec2(0.0f);

    // multiply the world space position with the inverse of the previous frame VP matrix
    vec4 posCSPrev = cameraUBO.matVPPrev * vec4(inPosWS,1.0f);

    vec3 posNDC = (inPosCS.xyz / inPosCS.w) * 0.5 + 0.5;
    vec3 posNDCPrev = (posCSPrev.xyz / posCSPrev.w) * 0.5 + 0.5; 

    motionVec = posNDCPrev.xy - posNDC.xy;

    return motionVec;
}

void main() {

    Material mat = materialUBO.materials[pcs.matIndex];
    ShadeParams params = unpackMaterial(mat, inNormal, inTBN, inTexCoord);
    
    outAlbedo = vec4(params.albedo, params.metallic); // smuggle metallic into albedo texture
    outNormal = vec4(params.normal, params.roughness); // smuggle roughness into normal texture
    outMotion = getMotionVector();
    outMeshId = pcs.meshId;
    outMaterialId = pcs.matIndex;
}