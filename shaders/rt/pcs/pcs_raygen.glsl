#ifndef PCS_RAYGEN_GLSL
#define PCS_RAYGEN_GLSL


layout(push_constant, std140) uniform PushConstants {
    uint albedoMapHandle;
    uint normalMapHandle;
    uint depthMapHandle;
    uint materialMapHandle;

    uint targetHandle;
    uint skyHandle;
    uint skyCdfHandle;

    uint seed;
    uint accumulate;
    uint frameCtr;

    uint maxRecursionDepth;

    uint doRIS;
    uint M_brdf;
    uint M_area;
    uint M_env;
} pcs;


#endif // PCS_RAYGEN_GLSL
