//https://github.com/yknishidate/single-file-vulkan-pathtracing
//https://nvpro-samples.github.io/vk_raytracing_tutorial_KHR/

#version 460
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_ray_tracing : require

#include "../common/common.glsl"
#include "../common/math_constants.glsl"
#include "raycommon.glsl"
#include "structs/payload.glsl"

layout(location = 0) rayPayloadInEXT HitPayload payload;
hitAttributeEXT vec2 attribs;

vec3 getPositionWS(vec3 uv, Vertex v0, Vertex v1, Vertex v2){
    vec3 pos = uv.x * v0.position + uv.y * v1.position + uv.z * v2.position;
    vec3 posWS = vec3(gl_ObjectToWorldEXT * vec4(pos, 1.0));
    return posWS;
}

vec3 getNormalWS(vec3 uv, Vertex v0, Vertex v1, Vertex v2){
    vec3 normal = uv.x * v0.normal + uv.y * v1.normal + uv.z * v2.normal;
    mat3 N = transpose(inverse(mat3(gl_ObjectToWorldEXT)));
    return (N * normal);
}

vec3 getTangentWS(vec3 uv, Vertex v0, Vertex v1, Vertex v2){
    vec3 tangent = uv.x * v0.tangent + uv.y * v1.tangent + uv.z * v2.tangent;
    vec3 tangentWS = normalize(vec3(tangent * gl_WorldToObjectEXT));
    return tangentWS;
}

vec2 getTexCoord(vec3 uv, Vertex v0, Vertex v1, Vertex v2){
    vec2 texCoord = uv.x * v0.texCoord + uv.y * v1.texCoord + uv.z * v2.texCoord;
    return texCoord;
}

void main() {
    ObjDesc object = objDesc.i[gl_InstanceCustomIndexEXT];

    Material material = materialUBO.materials[object.materialId];
    Indices indices = Indices(object.indexBufferAddr);
    Vertices vertices = Vertices(object.vertexBufferAddr);

    ivec3 ind = indices.i[gl_PrimitiveID];
    Vertex v0 = vertices.v[ind.x];
    Vertex v1 = vertices.v[ind.y];
    Vertex v2 = vertices.v[ind.z];

    vec3 uv = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);
    vec2 texCoord = getTexCoord(uv, v0, v1, v2);
    vec3 hitNormal = getNormalWS(uv, v0, v1, v2);
    vec3 posWS = getPositionWS(uv, v0, v1, v2);

    if (dot(-gl_WorldRayDirectionEXT,hitNormal) < 0.0)
        hitNormal *= -1.0;

    vec3 hitTangent = getTangentWS(uv, v0, v1, v2);

    vec3 T = hitTangent - dot(hitTangent, hitNormal) * hitNormal;
    T = normalize(T);
    vec3 B = normalize(cross(hitNormal, T));
    mat3 TBN = mat3(T, B, hitNormal);

    ShadeParams params = unpackMaterial(material, hitNormal, TBN, texCoord);

    // flip normal if backside is hit
    if (dot(-gl_WorldRayDirectionEXT,params.normal) < 0.0)
        params.normal *= -1.0;

    payload.hitPosition = posWS;
    payload.hitNormal = params.normal;
    payload.hitEmission = material.emission;
    payload.hitBrdf = params.albedo * INVPI;
    payload.hit = true;
}