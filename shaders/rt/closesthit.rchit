#version 460
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_ray_tracing : require

#include "../common/common.glsl"
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
    vec3 normalWS = normalize(vec3(normal * gl_WorldToObjectEXT));
    return normalWS;
}

void main()
{
/*
    payload.brdf = face.diffuse / M_PI;
    payload.emission = face.emission;
    payload.position = position;
    payload.normal = normal;*/

    ObjDesc object = objDesc.i[gl_InstanceCustomIndexEXT];

    Material material = materialUBO.materials[object.materialId];
    Indices indices = Indices(object.indexBufferAddr);
    Vertices vertices = Vertices(object.vertexBufferAddr);

    ivec3 ind = indices.i[gl_PrimitiveID];
    Vertex v0 = vertices.v[ind.x];
    Vertex v1 = vertices.v[ind.y];
    Vertex v2 = vertices.v[ind.z];

    vec3 uv = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);

    vec3 posWS = getPositionWS(uv, v0, v1, v2);
    vec3 normal = getNormalWS(uv, v0, v1, v2);

    payload.position = normal;
}