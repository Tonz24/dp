#include "math_constants.glsl"

//====================MATERIAL====================
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
ShadeParams unpackMaterial(Material mat,vec3 normal, mat3 tbn, vec2 texCoord){
    ShadeParams params;

    float hasAlbedoMap = clamp(float(mat.diffuseAlbedoMapHandle),0.0f,1.0f);
    params.albedo = mix(mat.diffuseAlbedo, texture(textures[mat.diffuseAlbedoMapHandle], texCoord).rgb, hasAlbedoMap);

    float hasNormalMap = clamp(float(mat.normalMapHandle),0.0f,1.0f);
    params.normal = mix(normalize(normal),normalize(tbn * (texture(textures[mat.normalMapHandle],texCoord).xyz * 2.0 - 1.0)),hasNormalMap);

    float hasShininessMap = clamp(float(mat.shininessMapHandle),0.0f,1.0f);
    params.shininess = mix(mat.shininess, texture(textures[mat.shininessMapHandle], texCoord).r, hasShininessMap);
    // roughness to shininess remapping https://simonstechblog.blogspot.com/2011/12/microfacet-brdf.html
    params.shininess = mix(params.shininess,2.0f / (params.shininess * params.shininess) - 2.0f,hasShininessMap);

    return params;
}
//================================================


vec2 dirToUv(vec3 dir){
    const float u = 0.5f + 0.5f * atan(dir.z, dir.x) * INVPI;
    const float v = 1.0f - acos(dir.y) * INVPI;
    return vec2(u,v);
}

vec3 sampleSphericalMap(vec3 dir, uint skyTextureIndex){
    vec2 uv = dirToUv(dir);
    return texture(textures[skyTextureIndex],uv).xyz;
}