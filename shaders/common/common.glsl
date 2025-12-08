#ifndef COMMON_GLSL
#define COMMON_GLSL

#include "math_constants.glsl"
#include "material.glsl"
#include "descriptor.glsl"

//=============GLOBAL DESCRIPTOR SET==============
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

ShadeParams unpackMaterial(Material mat, vec3 normal, mat3 tbn, vec2 texCoord){
    ShadeParams params;

    bool hasAlbedoMap = mat.albedoMapHandle > 0;
    params.albedo = hasAlbedoMap ? texture(textures[mat.albedoMapHandle], texCoord).rgb :  mat.albedoRoughness.rgb;

    bool hasNormalMap = mat.normalMapHandle > 0;
    params.normal = hasNormalMap ? normalize(tbn * (texture(textures[mat.normalMapHandle],texCoord).xyz * 2.0 - 1.0)) : normalize(normal);

    bool hasRoughnessMap = mat.roughnessMapHandle > 0;
    params.roughness = hasRoughnessMap ? texture(textures[mat.roughnessMapHandle], texCoord).r : mat.albedoRoughness.w;

    bool hasMetallicMap = mat.metallicMapHandle > 0;
    params.metallic = hasMetallicMap ? texture(textures[mat.metallicMapHandle], texCoord).r : mat.emissionMetallic.w;

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

vec3 sanitize(vec3 val){

    bool isValid = !(any(isinf(val)) || any(isnan(val)) || any(lessThan(val, vec3(0.0))));
    
    uvec3 valUnsigned = floatBitsToUint(val) & (uint(isValid) * uint(0xFFFFFFFF));
    vec3 valSanitized = uintBitsToFloat(valUnsigned);

    return valSanitized;
}

float sanitize(float val){

    bool isValid = !(isinf(val) || isnan(val) || val < 0.0f);

    uint valUnsigned = floatBitsToUint(val) & (uint(isValid) * uint(0xFFFFFFFF));
    float valSanitized = uintBitsToFloat(valUnsigned);

    return valSanitized;
}

//  all zero normals are invalid
bool isGeometryValid(vec3 normal){
    float epsilon = 1e-6;
    return any(greaterThan(abs(normal),vec3(epsilon)));
}

vec3 reconstructPositionWS(vec2 uv, float depth){
    uv.y = 1.0 - uv.y;
    vec2 ndcXY = uv * 2.0 - 1.0; // move from [0, 1] to [-1, 1] range   
    vec4 camRay = cameraUBO.matInvVP * vec4(ndcXY,depth,1);
    return camRay.xyz / camRay.w;
}

uint flatten2DCoord(uvec2 coord, uvec2 imgSize){
    return imgSize.x * coord.y + coord.x;
}

#endif // COMMON_GLSL