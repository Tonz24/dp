#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inTangent;
layout(location = 3) in vec2 inTexCoord;

layout(location = 0) out vec3 outFragColor;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec2 outTexCoord;

layout(push_constant) uniform PushConstants {
    mat4 matM;
    mat4 matN;
    uint matIndex;
} pcs;

#include "common.glsl"

void main() {
    gl_Position = cameraUBO.matVP * pcs.matM * vec4(inPosition,1);

    outNormal = normalize(mat3(pcs.matN) * inNormal);
    outFragColor = vec3(cameraUBO.posWS);
    outTexCoord = inTexCoord;
}