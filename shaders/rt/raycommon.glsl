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
    CDFElement cdf[];
} emissiveCDF;

// tests visibility between points a and b using a ray query
// avoids touching the SBT at all (should reduce unwanted overhead)
bool isVisible(vec3 a, vec3 b, vec3 normal){

    float tMin = 0.01;

    vec3 toLightUnnorm = b - a;
    vec3 dirToLight = normalize(toLightUnnorm);
    float dstToLight = max(length(toLightUnnorm) - tMin*1.5,tMin);

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


vec3 evaluateDirectLighting(vec3 albedo, vec3 posWS, vec3 normal, inout uint seed){
    // take CDF sample
    uint cdfSampleIndex = sampleCDFIndex(rand(seed));
    CDFElement cdfSample = emissiveCDF.cdf[cdfSampleIndex];

    TrianglePacked cdfTriangle = emissiveBuffer.tris[cdfSample.triIndex];
    vec3 L_i = vec3(cdfTriangle.v0eR.w,cdfTriangle.v1eG.w,cdfTriangle.v2eB.w);

    // sample the triangle identified by CDF sample
    TriangleSample sampledPoint = sampleTriangle(cdfTriangle,seed);
    float pdfCDF = cdfSampleIndex == 0 ? cdfSample.cdfVal : cdfSample.cdfVal - emissiveCDF.cdf[cdfSampleIndex-1].cdfVal;
    float pdf = sampledPoint.pdf * pdfCDF;

    // early exit if occlusion test fails as there wouldn't be any light contribution anyway
    if (!isVisible(posWS, sampledPoint.position, normal))
    return vec3(0.0);

    vec3 omega_i = sampledPoint.position - posWS;
    float r_sqr = dot(omega_i, omega_i);
    omega_i = normalize(omega_i);

    if (r_sqr == 0.0 || any(isnan(omega_i)) || any(isinf(omega_i)))
    return vec3(0.0);

    float cos_theta_i = max(dot(omega_i, normal),0.0);
    float cos_theta_y = max(dot(-omega_i, sampledPoint.normal),0.0);

    float G = cos_theta_i * cos_theta_y / r_sqr;
    vec3 brdf = albedo / PI;

    return L_i * brdf * G / pdf;
}