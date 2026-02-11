#version 460
#extension GL_GOOGLE_include_directive : enable

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inTangent;
layout(location = 3) in vec2 inTexCoord;

layout(location = 0) out vec3 outNormal;
layout(location = 1) out vec2 outTexCoord;
layout(location = 2) out vec3 outPosWS;
layout(location = 3) out vec4 outPosCS;
layout(location = 4) out mat3 outTBN;

#include "../common/common.glsl"
#include "pcs/pcs_gbuffer_fill.glsl"

void main() {
    vec4 posWS = pcs.matM * vec4(inPosition,1.0);
    outPosWS = posWS.xyz;
    outPosCS = cameraUBO.matVP * posWS; // save the clip space position withour perspective divide
    gl_Position = outPosCS; // perspective divide happens automatically

    outNormal = normalize(mat3(pcs.matN) * inNormal);
    vec3 tangent = normalize(mat3(pcs.matN) * inTangent);

    vec3 T = normalize(tangent - dot(tangent, outNormal) * outNormal); // Gram-Schmidt
    vec3 B = normalize(cross(outNormal,T));
    outTBN = mat3(T, B, outNormal);

    outTexCoord = inTexCoord;
}