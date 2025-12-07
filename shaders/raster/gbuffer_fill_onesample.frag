#version 460
#extension GL_GOOGLE_include_directive : require

layout(location = 0) out uint outMeshId;
layout(location = 1) out uint outMaterialId;

#include "../common/common.glsl"
#include "../common/rng.glsl"
#include "pcs/pcs_gbuffer_fill.glsl"

void main() {
    outMeshId = pcs.meshId;
    outMaterialId = pcs.matIndex;
}