#include "structs/structs.glsl"
#include "sampling.glsl"

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

struct TriangleSample{
    vec3 position;
    vec3 normal;
    float pdf;
};

struct CDFElement {
    uint triIndex;
    float cdfVal;
};


layout(set = 0, binding = 5, scalar) readonly buffer ObjDesc_ { ObjDesc i[]; } objDesc;

layout(set = 0, binding = 7, std430) readonly buffer EmissiveTriangles {
    uint size;
    TrianglePacked tris[];
} emissiveBuffer;

layout(set = 0, binding = 8, std430) readonly buffer EmissiveCDF {
    uint size;
    float area;
    CDFElement cdf[];
} emissiveCDF;

// tests visibility between points a and b using a ray query
// avoids touching the SBT at all (should reduce unwanted overhead)
bool isVisible(vec3 a, vec3 b){

    float tMin = 0.01;

    vec3 toLightUnnorm = b - a;
    vec3 dirToLight = normalize(toLightUnnorm);
    float dstToLight = max(length(toLightUnnorm) - tMin* 1.5, tMin);

    // tmax is distance to light - if the ray hits anything, the surface is NOT visible, otherwise it is
    rayQueryEXT query;
    rayQueryInitializeEXT(
        query,
        topLevelAS,
        gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT,
        0xFF,
        a,
        tMin,
        dirToLight,
        dstToLight
    );
    rayQueryProceedEXT(query);

    return rayQueryGetIntersectionTypeEXT(query, true) == gl_RayQueryCommittedIntersectionNoneEXT;
};


uint sampleCDFIndex(float rnd){

    if (emissiveCDF.size == 0)
        return uint(0xFFFFFFFF);

    uint left = 0;
    uint right = emissiveCDF.size - 1;

    while (left < right){
        uint mid = left + (right - left) / 2;

        if (rnd < emissiveCDF.cdf[mid].cdfVal)
            right = mid;
        else
            left = mid + 1;
    }
    return left;
}


TriangleSample sampleTriangle(TrianglePacked tri, inout uint seed){
    float r1 = rand(seed);
    float r2 = rand(seed);

    float x = 1.0f - sqrt(r1);
    float y = sqrt(r1) * (1.0f - r2);
    float z = sqrt(r1) * r2;

    TriangleSample s;
    //  barycentric interpolation of hit position
    s.position = tri.v0eR.xyz * x + tri.v1eG.xyz * y + tri.v2eB.xyz * z;
    // calculate normal
    s.normal = normalize(cross(normalize(tri.v1eG.xyz - tri.v0eR.xyz),normalize(tri.v2eB.xyz - tri.v0eR.xyz)));
    // pdf of hitting this point on the triangle is always 1.0/area (the probablility of choosing any point is uniform)
    s.pdf = 1.0f / tri.area;
    return s;
}
