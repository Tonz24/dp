#version 460
#extension GL_GOOGLE_include_directive : enable

layout(location = 0) in vec2 inNDCxy;

layout(location = 0) out vec4 fragColor;


#include "../common/common.glsl"
#include "../common/tonemappers.glsl"
#include "../common/math_constants.glsl"
#include "pcs/pcs_tonemap.glsl"


bool tonemap(vec2 uv){
    vec3 normal = texture(textures[pcs.normalTexIndex], uv).xyz;
    return any(notEqual(normal,vec3(0.0)));
}

void main() {

    vec2 screenTexCoord = inNDCxy * 0.5 + 0.5;
    screenTexCoord.y = 1.0 - screenTexCoord.y;

    vec3 rawColor = texture(textures[pcs.accumulatorIndex], screenTexCoord).xyz;

    // tonemap only if there's a valid normal at this texel coordinate
    // texels without a valid normal contain environment map data, which is already tonemapped
    vec3 color = tonemap(screenTexCoord) && pcs.doTonemap == 1 ? aces(rawColor) : rawColor;

    fragColor = vec4(color,1.0);
}