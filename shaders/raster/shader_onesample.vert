#version 460
#extension GL_GOOGLE_include_directive : require

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inTangent;
layout(location = 3) in vec2 inTexCoord;

#include "../common/common.glsl"
#include "pcs/pcs_gbuffer_fill.glsl"

void main() {
    vec4 posWS = pcs.matM * vec4(inPosition,1.0);
    gl_Position = cameraUBO.matVP * posWS;

}