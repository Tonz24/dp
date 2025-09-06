#version 450

layout(location = 0) in vec2 inNDCxy;

layout(location = 0) out vec4 fragColor;

layout(set = 1, binding = 0) uniform sampler2D albedoMap;
layout(set = 1, binding = 1) uniform sampler2D normalMap;
layout(set = 1, binding = 2) uniform sampler2D depthMap;
layout(set = 1, binding = 3) uniform usampler2D materialMap;

#include "common.glsl"
#include "pcs_gbuffer_shade.glsl"
#include "tonemappers.glsl"
#include "math_constants.glsl"

const int OVERLAY_DEBUG_PHONG = 0;
const int OVERLAY_ALBEDO_MAP = 1;
const int OVERLAY_NORMAL_MAP = 2;
const int OVERLAY_DEPTH_MAP = 3;
const int OVERLAY_WS_POS = 4;


//  https://stackoverflow.com/questions/51108596/linearize-depth
float linearizeDepth(float depth, float zNear, float zFar){
    return zNear * zFar / (zFar + depth * (zNear - zFar));
}


void main() {
    vec2 screenTexCoord = inNDCxy * 0.5 + 0.5;
    screenTexCoord.y = 1.0 - screenTexCoord.y;

    vec4 albedoTexCoordX = texture(albedoMap, screenTexCoord);
    vec4 normalTexCoordY = texture(normalMap, screenTexCoord);
    float depth = texture(depthMap, screenTexCoord).x;
    uint materialId = texture(materialMap, screenTexCoord).x;

    vec3 albedo = albedoTexCoordX.xyz;
    vec3 normal = normalTexCoordY.xyz;

    vec2 texCoord = vec2(albedoTexCoordX.w,normalTexCoordY.w);

    Material mat = materialUBO.materials[materialId];

    float hasAlbedoMap = clamp(float(mat.diffuseAlbedoMapHandle),0.0f,1.0f);
    albedo = mix(mat.diffuseAlbedo, texture(textures[mat.diffuseAlbedoMapHandle], texCoord).rgb, hasAlbedoMap);


    //  this fragment has something to shade only if there's a normal behind it
    bool hasValidGeometry = (bool((normal.x > 0 || normal.x < 0) || (normal.y > 0 || normal.y < 0) || (normal.z > 0 || normal.z < 0)));
    if (!hasValidGeometry) {
        if (pcs.drawSkybox == 0){
            fragColor = vec4(0.0,0.0,0.0,1.0);
            return;
        }
        discard;
    }

    vec4 camRay = cameraUBO.matInvVP * vec4(inNDCxy,depth,1);
    vec3 posWS = camRay.xyz / camRay.w;

    if (pcs.overlayIndex == OVERLAY_ALBEDO_MAP)
        fragColor = vec4(albedo,1.0);
    if (pcs.overlayIndex == OVERLAY_NORMAL_MAP){
        vec3 normalRemapped = normal * 0.5 + 0.5;
        fragColor = vec4(mix(normal,normalRemapped,float(pcs.remapNormals)),1.0);
    }
    if (pcs.overlayIndex == OVERLAY_DEPTH_MAP){
        float linDepth = linearizeDepth(depth, cameraUBO.zNear, cameraUBO.zFar);
        fragColor = vec4(linDepth, linDepth, linDepth,1.0);
    }
    if (pcs.overlayIndex == OVERLAY_WS_POS){
        fragColor = vec4(texCoord,0,1);
        //fragColor = vec4(posWS,1);
    }

    if (pcs.overlayIndex == OVERLAY_DEBUG_PHONG){
        vec4 camRay = cameraUBO.matInvVP * vec4(inNDCxy,depth,1);

        vec3 L_unnorm = pcs.lightPosWS - posWS;
        float inv_r_sqr = 1.0 / dot(L_unnorm,L_unnorm);
        vec3 L = normalize(L_unnorm);
        vec3 V = normalize(cameraUBO.posWS - posWS);
        vec3 R = normalize(reflect(-V,normal));

        float shininess = materialUBO.materials[materialId].shininess;

        float cos_theta_r = max(dot(R,L),0.0);
        float cos_theta_i = max(dot(L,normal),0.0);

        vec3 albedoSpec = vec3(1) - albedo;
        vec3 f_r = albedo * INVPI + INV2PI * albedoSpec * (shininess + 2) * pow(cos_theta_r,shininess);
        vec3 L_i = pcs.lightEmission;

        vec3 L_o = L_i * f_r * cos_theta_i * inv_r_sqr;

        fragColor = vec4(aces(L_o),1);
    }
}