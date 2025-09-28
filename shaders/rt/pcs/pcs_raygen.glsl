layout(push_constant, std140) uniform PushConstants {
    uint albedoMapHandle;
    uint normalMapHandle;
    uint depthMapHandle;
    uint materialMapHandle;

    uint targetHandle;
    uint skyHandle;

    uint seed;
    uint accumulate;
    uint frameCtr;

    uint maxRecursionDepth;
    uint sampleSky;
    uint NEE;
} pcs;