#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inTangent;
layout(location = 3) in vec2 inTexCoord;

layout(location = 0) out vec3 outNormal;
layout(location = 1) out vec2 outTexCoord;
layout(location = 2) out mat3 outTBN;


#include "common.glsl"

void main() {
    gl_Position = cameraUBO.matVP * pcs.matM * vec4(inPosition,1);

    outNormal = normalize(mat3(pcs.matN) * inNormal);
    vec3 tangent = normalize(mat3(pcs.matN) * inTangent);

    vec3 T = normalize(tangent - dot(tangent, outNormal) * outNormal); // Gram-Schmidt
    vec3 B = normalize(cross(outNormal,T));
    outTBN = mat3(T, B, outNormal);

    outTexCoord = inTexCoord;
}