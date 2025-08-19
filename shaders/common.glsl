layout (set=0, binding=0, std140) uniform CameraUBO {
    mat4 matV;
    mat4 matP;
    mat4 matVP;
    mat4 matInvVP;
    vec3 posWS;
} cameraUBO;


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
    float padding;
    float padding2;
};

layout (set=0,binding=1, std140) uniform MaterialUBO {
   Material materials[100];
} materialUBO;

layout(push_constant) uniform PushConstants {
    mat4 matM;
    mat4 matN;
    uint matIndex;
    uint meshId;
} pcs;