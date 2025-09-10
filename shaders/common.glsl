layout (set=0, binding=0, std140) uniform CameraUBO {
    mat4 matV;
    mat4 matP;
    mat4 matVP;
    mat4 matInvVP;
    vec3 posWS;
    float zNear;
    float zFar;
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

layout(set = 0, binding = 2) uniform sampler2D textures[1024];


layout(set = 0, binding = 3) uniform accelerationStructureEXT topLevelAS;