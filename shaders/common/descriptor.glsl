#ifndef DESCRIPTOR_GLSL
#define DESCRIPTOR_GLSL

#include "reservoir.glsl"

layout (set=0, binding=0, std140) uniform CameraUBO {
    mat4 matV;
    mat4 matP;
    mat4 matVP;
    mat4 matInvVP;

    mat4 matVPrev;
    mat4 matPPrev;
    mat4 matVPPrev;
    mat4 matInvVPPrev;

    vec3 posWS;
    float zNear;

    vec3 posWSPrev;
    float zFar;

} cameraUBO;

layout (set=0,binding=1, std140) uniform MaterialUBO {
    Material materials[100];
} materialUBO;

layout(set = 0, binding = 2) uniform sampler2D textures[1024];

#extension GL_EXT_ray_query : require
layout(set = 0, binding = 3) uniform accelerationStructureEXT topLevelAS;

layout(set = 0, binding = 4, rgba32f) uniform image2D outputImage;

layout(set = 0, binding = 6) uniform usampler2D utextures[1024];


struct Vertex {
    vec3 position;
    vec3 normal;
    vec3 tangent;
    vec2 texCoord;
};

#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_buffer_reference : require
layout(buffer_reference, scalar) readonly buffer Vertices {Vertex v[];};
layout(buffer_reference, scalar) readonly buffer Indices {ivec3 i[];};

struct ObjDesc {
    Vertices vertexBufferAddr;
    Indices indexBufferAddr;
    uint materialId;
    mat3 normalMat;
};
layout(set = 0, binding = 5, scalar) readonly buffer ObjDesc_ { ObjDesc i[]; } objDesc;


// fourth components together make RGB emission of the triangle
struct TrianglePacked {
   vec4 v0eR;
    vec4 v1eG;
    vec4 v2eB;
    vec4 n0eA;
    vec4 n1;
    vec4 n2;
};
layout(set = 0, binding = 7, std430) readonly buffer EmissiveTriangles {
    uint size;
    TrianglePacked tris[];
} emissiveBuffer;


struct CDFElement {
    uint triIndex;
    float cdfVal;
};

layout(set = 0, binding = 8, std430) readonly buffer EmissiveCDF {
    uint size;
    float area;
    CDFElement cdf[];
} emissiveCDF;


layout(set = 0, binding = 9, std430)  buffer ReservoirBuffer {
    Reservoir reservoirs[];
} reservoirBuffers[2];

#endif // DESCRIPTOR_GLSL