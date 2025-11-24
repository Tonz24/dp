#ifndef RAYCOMMON_GLSL
#define RAYCOMMON_GLSL

#include "sampling.glsl"
#include "../common/descriptor.glsl"

// tests visibility between points a and b using a ray query
// avoids touching the SBT at all (should reduce unwanted overhead)
bool isVisible(vec3 a, vec3 b, vec3 normalA){

    float tMin = 0.001;

    vec3 toLightUnnorm = b - a;
    vec3 dirToLight = normalize(toLightUnnorm);
    float dstToLight = length(toLightUnnorm) - tMin * 3.0f;
    //float dstToLight = max( - tMin* 3, tMin);

    // tmax is distance to light - if the ray hits anything, the surface is NOT visible, otherwise it is
    rayQueryEXT query;
    rayQueryInitializeEXT(
        query,
        topLevelAS,
        gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT,
        0xFF,
        a + tMin * normalA,
        tMin,
        dirToLight,
        dstToLight
    );
    rayQueryProceedEXT(query);

    while (rayQueryProceedEXT(query)) { }

    return rayQueryGetIntersectionTypeEXT(query, true) == gl_RayQueryCommittedIntersectionNoneEXT;
};

vec3 getPositionWS(mat4x3 modelMat, vec3 uv, Vertex v0, Vertex v1, Vertex v2){
    vec3 pos = uv.x * v0.position + uv.y * v1.position + uv.z * v2.position;
    vec3 posWS = vec3(modelMat * vec4(pos, 1.0));
    return posWS;
}

vec3 getNormalWS(mat3 normalMat,vec3 uv, Vertex v0, Vertex v1, Vertex v2){
    vec3 normal = uv.x * v0.normal + uv.y * v1.normal + uv.z * v2.normal;
    return (normalMat * normal);
}

vec3 getTangentWS(mat3 normalMat, vec3 uv, Vertex v0, Vertex v1, Vertex v2){
    vec3 tangent = uv.x * v0.tangent + uv.y * v1.tangent + uv.z * v2.tangent;
    return (normalMat * tangent);
}

vec2 getTexCoord(vec3 uv, Vertex v0, Vertex v1, Vertex v2){
    vec2 texCoord = uv.x * v0.texCoord + uv.y * v1.texCoord + uv.z * v2.texCoord;
    return texCoord;
}


ShadeParams resolveHit(uint instIndex, uint primId, vec2 attribs, vec3 rayDir, mat4x3 modelMat, inout vec3 posWS, inout vec3 emission){
    ObjDesc object = objDesc.i[instIndex];

    Material material = materialUBO.materials[object.materialId];
    Indices indices = Indices(object.indexBufferAddr);
    Vertices vertices = Vertices(object.vertexBufferAddr);

    ivec3 ind = indices.i[primId];
    Vertex v0 = vertices.v[ind.x];
    Vertex v1 = vertices.v[ind.y];
    Vertex v2 = vertices.v[ind.z];

    mat3 normalMat = object.normalMat;

    vec3 uv = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);

    posWS = getPositionWS(modelMat, uv, v0, v1, v2);
    emission = material.emissionMetallic.rgb;

    vec2 texCoord = getTexCoord(uv, v0, v1, v2);
    vec3 hitNormal = getNormalWS(normalMat, uv, v0, v1, v2);
    vec3 hitTangent = getTangentWS(normalMat, uv, v0, v1, v2);
    
    vec3 T = hitTangent - dot(hitTangent, hitNormal) * hitNormal;
    T = normalize(T);
    vec3 B = normalize(cross(hitNormal, T));
    mat3 TBN = mat3(T, B, hitNormal);

    ShadeParams params = unpackMaterial(material, hitNormal, TBN, texCoord);

    // flip normal if backside is hit
    if (dot(-rayDir,params.normal) < 0.0)
        params.normal *= -1.0;

    return params;
}

#endif // RAYCOMMON_GLSL