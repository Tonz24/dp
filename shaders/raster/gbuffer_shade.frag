#version 460
#extension GL_EXT_ray_query : require
#extension GL_GOOGLE_include_directive : enable

layout(location = 0) in vec2 inNDCxy;

layout(location = 0) out vec4 fragColor;

#include "../common/common.glsl"
#include "pcs/pcs_gbuffer_shade.glsl"
#include "../common/tonemappers.glsl"
#include "../common/math_constants.glsl"

const int OVERLAY_DEBUG_PHONG = 0;
const int OVERLAY_ALBEDO_MAP = 1;
const int OVERLAY_NORMAL_MAP = 2;
const int OVERLAY_DEPTH_MAP = 3;
const int OVERLAY_WS_POS = 4;


//  https://stackoverflow.com/questions/51108596/linearize-depth
float linearizeDepth(float depth, float zNear, float zFar){
    return zNear * zFar / (zFar + depth * (zNear - zFar));
}

/** https://github.com/KhronosGroup/Vulkan-Samples/blob/main/shaders/ray_queries/glsl/ray_shadow.frag
 * @brief Calculates shadow factors for given hit point and light position. If in shadow, the diffuse factor is 0.25 to prevent the surface from appearing pitch black
 * @param posWS world space hit point
 * @param lightPos world space light position
 * @return vec2(diffuseFactor,specularFactor)
 */
vec2 getShadowFactor(vec3 posWS, vec3 lightPos){

    vec3 toLightUnnorm = lightPos - posWS;
    vec3 dirToLight = normalize(toLightUnnorm);
    float dstToLight = length(toLightUnnorm);
    float tMin = 0.01;


    // tmax is distance to light - if the ray hits anything, the surface is in shadow, otherwise the light is visible
    rayQueryEXT query;
    rayQueryInitializeEXT(query,topLevelAS,gl_RayFlagsTerminateOnFirstHitEXT,0xFF,posWS,tMin,dirToLight,dstToLight);
    rayQueryProceedEXT(query);

    vec2 inShadow = vec2(0.25f,0.0f);
    vec2 noShadow = vec2(1.0f);

    return mix(noShadow,inShadow,float((rayQueryGetIntersectionTypeEXT(query, true) != gl_RayQueryCommittedIntersectionNoneEXT)));
}


void main() {
    vec2 screenTexCoord = inNDCxy * 0.5 + 0.5;
    screenTexCoord.y = 1.0 - screenTexCoord.y;

    vec3 albedo = texture(textures[pcs.albedoMapHandle], screenTexCoord).xyz;
    vec4 normalShininess = texture(textures[pcs.normalMapHandle], screenTexCoord);
    vec3 normal = normalize(normalShininess.xyz);
    float shininess = normalShininess.w;
    float depth = texture(textures[pcs.depthMapHandle], screenTexCoord).x;


    //  this fragment has something to shade only if there's a valid normal underneath it
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
        float linDepth = linearizeDepth(depth, cameraUBO.zNear, cameraUBO.zFar) * 0.125;
        fragColor = vec4(linDepth, linDepth, linDepth,1.0);
    }
    if (pcs.overlayIndex == OVERLAY_WS_POS){
        fragColor = vec4(posWS,1);
    }

    if (pcs.overlayIndex == OVERLAY_DEBUG_PHONG){
        vec4 camRay = cameraUBO.matInvVP * vec4(inNDCxy,depth,1);

        vec3 L_unnorm = pcs.lightPosWS - posWS;
        float inv_r_sqr = 1.0 / dot(L_unnorm,L_unnorm);
        float dstToLight = length(L_unnorm);
        vec3 L = normalize(L_unnorm);
        vec3 V = normalize(cameraUBO.posWS - posWS);
        vec3 R = normalize(reflect(-V,normal));

        float cos_theta_r = max(dot(R,L),0.0);
        float cos_theta_i = max(dot(L,normal),0.0);

        vec2 shadow = getShadowFactor(posWS,pcs.lightPosWS);

        vec3 albedoSpec = vec3(1) - albedo;
        vec3 f_r_diffuse = albedo * INVPI * shadow.x;
        vec3 f_r_specular = INV2PI * albedoSpec * (shininess + 2) * pow(cos_theta_r,shininess) * shadow.y;
        vec3 f_r = f_r_diffuse + f_r_specular;
        vec3 L_i = pcs.lightEmission;

        vec3 L_o = L_i * f_r * cos_theta_i * inv_r_sqr;

        fragColor = vec4(aces(L_o),1);
    }
}