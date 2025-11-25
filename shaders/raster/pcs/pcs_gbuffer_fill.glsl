#ifndef PCS_GBUFFER_FILL_GLSL
#define PCS_GBUFFER_FILL_GLSL

layout(push_constant) uniform PushConstants {
    mat4 matM;
    mat4 matN;
    uint matIndex;
    uint meshId;

    uint seed;
    uint width;
    uint height;
} pcs;

#endif // PCS_GBUFFER_FILL_GLSL