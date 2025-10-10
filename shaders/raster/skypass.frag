#version 460
#extension GL_GOOGLE_include_directive : enable

layout(location = 0) in vec2 inNDCxy;

layout(location = 0) out vec4 fragColor;


#include "../common/common.glsl"
#include "../common/tonemappers.glsl"
#include "../common/math_constants.glsl"
#include "pcs/pcs_skypass.glsl"

void main() {
    vec4 camRay = cameraUBO.matInvVP * vec4(inNDCxy,1,1);
    vec3 dir = normalize(camRay.xyz / camRay.w - cameraUBO.posWS);

    vec3 envMapColor = sampleSphericalMap(dir, pcs.skyIndex);
    envMapColor = aces(envMapColor);

    fragColor = vec4(envMapColor,1.0);
}