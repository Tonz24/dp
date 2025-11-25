#ifndef MATERIAL_GLSL
#define MATERIAL_GLSL

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
    float roughness;
    float metallic;
};

#define MAT_DIFFUSE 0
#define MAT_MIRROR  1
#define MAT_PBR     2

#endif // MATERIAL_GLSL