layout(push_constant, std140) uniform PushConstants {
    vec3 lightPosWS;
    int overlayIndex;

    vec3 lightEmission;
    int drawSkybox;

    int remapNormals;
    uint albedoMapHandle;
    uint normalMapHandle;
    uint depthMapHandle;

    uint materialMapHandle;
} pcs;