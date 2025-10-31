//https://github.com/yknishidate/single-file-vulkan-pathtracing
//https://nvpro-samples.github.io/vk_raytracing_tutorial_KHR/

#version 460
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_ray_tracing : require

#include "../common/common.glsl"
#include "../common/math_constants.glsl"
#include "pcs/pcs_raygen.glsl"
#include "raycommon.glsl"
#include "mis.glsl"

layout(location = 0) rayPayloadInEXT HitPayload payload;

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

    // passed from raygen shader, put into a separate variable, otherwise there's VK_DEVICE_LOST if used directly as an inout parameter
    uint seed = payload.seed;

    vec4 nextSample = sampleHemisphereCosineWeighted(params.normal,seed);
    vec3 nextDir = nextSample.xyz;
    float pdf = nextSample.w;

    payload.hitPosition = posWS;
    payload.hitEmission = material.emission;
    vec3 hitBrdf = params.albedo * INVPI;
    payload.hit = true;
    payload.nextSample = nextSample;
    payload.hitNormal =  params.normal;

    payload.weightFactor = hitBrdf * max(dot(nextDir,params.normal),0.0) / pdf;

    if(!hitLight(payload))
        payload.directContribution = calculateDirect(params.albedo,posWS,params.normal,seed);

    payload.seed = seed;
    payload.mirror = false;
}