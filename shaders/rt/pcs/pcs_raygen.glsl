layout(push_constant, std140) uniform PushConstants {
    uint albedoMapHandle;
    uint normalMapHandle;
    uint depthMapHandle;
    uint materialMapHandle;

    uint skyHandle;

    uint seed;
    uint accumulate;
    uint frameCtr;
} pcs;