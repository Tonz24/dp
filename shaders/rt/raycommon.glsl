#include "structs/structs.glsl"

#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_buffer_reference : require
layout(buffer_reference, scalar) readonly buffer Vertices {Vertex v[];};
layout(buffer_reference, scalar) readonly buffer Indices {ivec3 i[];};

struct ObjDesc {
    Vertices vertexBufferAddr;
    Indices indexBufferAddr;
    uint materialId;
};

// fourth components together make RGB emission of the triangle
struct TrianglePacked {
    vec4 v0eR;
    vec4 v1eG;
    vec4 v2eB;
    float area;
};

struct TriangleUnpacked{
    vec3 v0;
    vec3 v1;
    vec3 v2;
    vec3 emission;
};


TriangleUnpacked unpackTriangle(TrianglePacked tri){
    TriangleUnpacked t;
    t.v0 = tri.v0eR.xyz;
    t.v1 = tri.v1eG.xyz;
    t.v2 = tri.v2eB.xyz;
    t.emission = vec3(tri.v0eR.w,tri.v1eG.w,tri.v2eB.w);
    return t;
}

TrianglePacked packTriangle(TriangleUnpacked tri){
    TrianglePacked t;
    t.v0eR = vec4(tri.v0,tri.emission.x);
    t.v1eG = vec4(tri.v1,tri.emission.y);
    t.v2eB = vec4(tri.v2,tri.emission.z);
    return t;
}


struct CDFElement {
    uint triIndex;
    float pdf;
};


layout(set = 0, binding = 5, scalar) buffer ObjDesc_ { ObjDesc i[]; } objDesc;

layout(set = 0, binding = 7, scalar) buffer EmissiveTriangles {
    uint size;
    TrianglePacked tris[];
} emissiveBuffer;

layout(set = 0, binding = 8, scalar) buffer EmissiveCDF {
    uint size;
    CDFElement cdf[];
} emissiveCDF;