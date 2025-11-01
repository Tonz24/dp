
#include "math_constants.glsl"

//====================MATERIAL====================
/*
struct Material{
    vec3 diffuseAlbedo;
    float shininess;

    vec3 specularAlbedo;
    float ior;

    vec3 emission;
    uint diffuseAlbedoMapHandle;

    vec3 attenuation;
    uint specularALbedoMapHandle;

    uint shininessMapHandle;
    uint normalMapHandle;
    uint materialType;
    float padding2;
};*/

struct Material{
    vec4 albedoRoughness;
    vec4 emissionMetallic;
    vec4 attenuationIor;

    uint albedoMapHandle;
    uint roughnessMapHandle;
    uint metallicMapHandle;
    uint normalMapHandle;

    uint materialType;
    float padding;
    float padding1;
    float padding2;
};


struct ShadeParams{
    vec3 albedo;
    vec3 normal;
    float shininess;
};
//================================================


//=============GLOBAL DESCRIPTOR SET==============
layout (set=0, binding=0, std140) uniform CameraUBO {
    mat4 matV;
    mat4 matP;
    mat4 matVP;
    mat4 matInvVP;
    vec3 posWS;
    float zNear;
    float zFar;
} cameraUBO;

layout (set=0,binding=1, std140) uniform MaterialUBO {
   Material materials[100];
} materialUBO;

layout(set = 0, binding = 2) uniform sampler2D textures[1024];
layout(set = 0, binding = 6) uniform usampler2D utextures[1024];

#extension GL_EXT_ray_query : require
layout(set = 0, binding = 3) uniform accelerationStructureEXT topLevelAS;
//================================================

//====================MATERIAL====================
//ShadeParams unpackMaterial(Material mat,vec3 normal, mat3 tbn, vec2 texCoord){
//    ShadeParams params;
//
//    float hasAlbedoMap = clamp(float(mat.diffuseAlbedoMapHandle),0.0f,1.0f);
//    params.albedo = mix(mat.diffuseAlbedo, texture(textures[mat.diffuseAlbedoMapHandle], texCoord).rgb, hasAlbedoMap);
//
//    float hasNormalMap = clamp(float(mat.normalMapHandle),0.0f,1.0f);
//    params.normal = mix(normalize(normal),normalize(tbn * (texture(textures[mat.normalMapHandle],texCoord).xyz * 2.0 - 1.0)),hasNormalMap);
//
//    float hasShininessMap = clamp(float(mat.shininessMapHandle),0.0f,1.0f);
//    params.shininess = mix(mat.shininess, texture(textures[mat.shininessMapHandle], texCoord).r, hasShininessMap);
//    // roughness to shininess remapping https://simonstechblog.blogspot.com/2011/12/microfacet-brdf.html
//    params.shininess = mix(params.shininess,2.0f / (params.shininess * params.shininess) - 2.0f,hasShininessMap);
//
//    return params;
//}

ShadeParams unpackMaterial(Material mat,vec3 normal, mat3 tbn, vec2 texCoord){
    ShadeParams params;

    bool hasAlbedoMap = mat.albedoMapHandle > 0;
    params.albedo = hasAlbedoMap ? texture(textures[mat.albedoMapHandle], texCoord).rgb :  mat.albedoRoughness.rgb;

    bool hasNormalMap = mat.normalMapHandle > 0;
    params.normal = hasNormalMap ? normalize(tbn * (texture(textures[mat.normalMapHandle],texCoord).xyz * 2.0 - 1.0)) : normalize(normal);

    bool hasShininessMap = mat.roughnessMapHandle > 0;
    params.shininess = hasShininessMap ? texture(textures[mat.roughnessMapHandle], texCoord).r : mat.albedoRoughness.w;

    // roughness to shininess remapping https://simonstechblog.blogspot.com/2011/12/microfacet-brdf.html
    params.shininess = 2.0f / (params.shininess * params.shininess) - 2.0f;

    return params;
}
//================================================


// calibrated so that test image (https://naver.github.io/egjs-view360/docs/projections/equirect) faces the camera when its view vector looks down (0, 0, -1) in world space
vec2 dirToEquirect(vec3 dir){
    const float u = 0.5f * atan(dir.z, dir.x) * INVPI - 0.25f;
    const float v = 1.0f - acos(dir.y) * INVPI;
    return vec2(u,v);
}

vec3 sampleSphericalMap(vec3 dir, uint skyTextureIndex){
    vec2 uv = dirToEquirect(dir);

    return texture(textures[skyTextureIndex],uv).xyz;
}