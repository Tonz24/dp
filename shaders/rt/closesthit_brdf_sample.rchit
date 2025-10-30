//https://github.com/yknishidate/single-file-vulkan-pathtracing
//https://nvpro-samples.github.io/vk_raytracing_tutorial_KHR/

#version 460
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_ray_tracing : require

#include "../common/common.glsl"
#include "../common/math_constants.glsl"
#include "raycommon.glsl"
#include "pcs/pcs_raygen.glsl"
#include "structs/payload.glsl"

layout(location = 1) rayPayloadInEXT BRDFSamplePayload inPayloadBRDF;

hitAttributeEXT vec2 attribs;


void main() {
    ObjDesc object = objDesc.i[gl_InstanceCustomIndexEXT];

    Material material = materialUBO.materials[object.materialId];
    Indices indices = Indices(object.indexBufferAddr);
    Vertices vertices = Vertices(object.vertexBufferAddr);

    ivec3 ind = indices.i[gl_PrimitiveID];
    Vertex v0 = vertices.v[ind.x];
    Vertex v1 = vertices.v[ind.y];
    Vertex v2 = vertices.v[ind.z];

    mat4x3 modelMat = gl_ObjectToWorldEXT;
    mat3 normalMat = transpose(inverse(mat3(gl_ObjectToWorldEXT)));

    vec3 uv = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);
    vec2 texCoord = getTexCoord(uv, v0, v1, v2);
    vec3 hitNormal = getNormalWS(normalMat, uv, v0, v1, v2);
    vec3 posWS = getPositionWS(modelMat, uv, v0, v1, v2);
    vec3 hitTangent = getTangentWS(normalMat, uv, v0, v1, v2);

    if (dot(-gl_WorldRayDirectionEXT,hitNormal) < 0.0)
        hitNormal *= -1.0;


    vec3 T = hitTangent - dot(hitTangent, hitNormal) * hitNormal;
    T = normalize(T);
    vec3 B = normalize(cross(hitNormal, T));
    mat3 TBN = mat3(T, B, hitNormal);

    ShadeParams params = unpackMaterial(material, hitNormal, TBN, texCoord);

    // flip normal if backside is hit
    if (dot(-gl_WorldRayDirectionEXT,params.normal) < 0.0)
        params.normal *= -1.0;

    vec3 ab = v1.position - v0.position;
    vec3 ac = v2.position - v0.position;

    inPayloadBRDF.hitPosition = posWS;
    inPayloadBRDF.hitNormal = params.normal;
    inPayloadBRDF.hitEmission = material.emission;
    inPayloadBRDF.hitArea = 0.5 * length(cross(ab, ac)) * float(any(greaterThan(inPayloadBRDF.hitEmission,vec3(0.0))));
    inPayloadBRDF.didHit = true;
}