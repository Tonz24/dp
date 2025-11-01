#version 460
#extension GL_EXT_ray_query : require
#extension GL_GOOGLE_include_directive : enable

layout(location = 0) in vec2 inNDCxy;

layout(location = 0) out vec4 fragColor;

#include "../common/common.glsl"
#include "pcs/pcs_gbuffer_shade.glsl"
#include "../common/tonemappers.glsl"
#include "../common/math_constants.glsl"
#include "../rt/raycommon.glsl"

const int OVERLAY_DEBUG_PHONG = 0;
const int OVERLAY_ALBEDO_MAP = 1;
const int OVERLAY_ROUGHNESS_MAP = 2;
const int OVERLAY_METALLIC_MAP = 3;
const int OVERLAY_NORMAL_MAP = 4;
const int OVERLAY_DEPTH_MAP = 5;
const int OVERLAY_WS_POS = 6;
const int OVERLAY_DEBUG_PBR = 7;


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
    rayQueryInitializeEXT(query,
                          topLevelAS,
                          gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsCullBackFacingTrianglesEXT,
                          0xFF,
                          posWS,
                          tMin,
                          dirToLight,
                          dstToLight);
    rayQueryProceedEXT(query);

    vec2 inShadow = vec2(0.25f,0.0f);
    vec2 noShadow = vec2(1.0f);

    return mix(noShadow,inShadow,float((rayQueryGetIntersectionTypeEXT(query, true) != gl_RayQueryCommittedIntersectionNoneEXT)));
}

vec3 fresnelSchlick(vec3 F_0, float cos_theta_h){
    float v = 1.0 - cos_theta_h;
    v = v * v * v * v * v; //fifth power
    return F_0 + (1.0 - F_0) * v;
}

float distrGGX(float alpha, float cos_theta_m){
    float a2 = alpha * alpha;
    float cos_theta_m2 = cos_theta_m * cos_theta_m;

    float term = (a2 - 1.0f) * cos_theta_m2 + 1;
    float term2 = term * term;

    float denom = PI * term2;
    
    return a2 / denom;
}

float aux(float cos_theta, float alpha){
    float a2 = alpha * alpha;
    float cos_theta2 = cos_theta * cos_theta;
    float term =  sqrt(1.0 + a2 * (1.0 / cos_theta2 - 1.0));

    return (term - 1.0) / 2.0;
}

float G(float cos_theta_o, float cos_theta_i, float alpha){
    float aux_theta_o = aux(cos_theta_o, alpha);
    float aux_theta_i = aux(cos_theta_i, alpha);

    return 1.0 / (1.0 + aux_theta_o + aux_theta_i);
}


vec3 evalPbr(vec3 albedo, float roughness, float metallic, vec3 normal, vec3 omega_o, vec3 omega_i, vec3 pos){

    vec3 omega_h = normalize(omega_i + omega_o); // halfway vector between view direction and light direction (microfacet normal)
    float cos_theta_m = max(dot(omega_h, normal), 0.0); // angle between microfacet normal and surface normal
    float cos_theta_h = max(dot(omega_h, omega_o), 0.0); // angle between microfacet normal and view direction

    float cos_theta_o = max(dot(omega_o, normal), 0.0); // angle between view direction and surface normal
    float cos_theta_i = max(dot(omega_i, normal), 0.0); // angle between light direction and surface normal

    //  Fresnel term
    vec3 F0 = mix(vec3(0.04), albedo, metallic);
    vec3 F = fresnelSchlick(F0, cos_theta_h);

    float alpha = roughness * roughness;

    float NDF = distrGGX(alpha, cos_theta_m);
    float G = G(cos_theta_o, cos_theta_i, alpha); 

    vec3 diff = albedo * INVPI;
    vec3 spec = (NDF * F * G) / (4.0 * cos_theta_o * cos_theta_i);

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;


    float distToLight = length(pcs.lightPosWS - pos);
    vec3 lightContrib = pcs.lightEmission / (distToLight * distToLight);  

    vec2 shadow = getShadowFactor(pos, pcs.lightPosWS);

    return (kD * diff * shadow.x + spec * shadow.y) * lightContrib * cos_theta_i;
}




void main() {

    vec2 screenTexCoord = inNDCxy * 0.5 + 0.5;
    screenTexCoord.y = 1.0 - screenTexCoord.y;

    vec4 albedoMetallic = texture(textures[pcs.albedoMapHandle], screenTexCoord);
    vec3 albedo = albedoMetallic.rgb;
    float metallic = albedoMetallic.a;

    vec4 normalRoughness = texture(textures[pcs.normalMapHandle], screenTexCoord);
    vec3 normal = normalize(normalRoughness.xyz);
    float roughness = normalRoughness.w;

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
    if (pcs.overlayIndex == OVERLAY_ROUGHNESS_MAP){
        fragColor = vec4(roughness,roughness,roughness,1);
    }
    if (pcs.overlayIndex == OVERLAY_METALLIC_MAP){
        fragColor = vec4(metallic,metallic,metallic,1);
    }

    if (pcs.overlayIndex == OVERLAY_DEBUG_PHONG){
        vec4 camRay = cameraUBO.matInvVP * vec4(inNDCxy,depth,1);
        float shininess = 2.0f / (roughness * roughness) - 2.0f;

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

    if (pcs.overlayIndex == OVERLAY_DEBUG_PBR){
        vec3 omega_o = normalize(cameraUBO.posWS - posWS);
        vec3 omega_i = normalize(pcs.lightPosWS - posWS);


        fragColor = vec4(evalPbr(albedo, roughness, metallic, normal, omega_o, omega_i, posWS),1.0);
    }
}