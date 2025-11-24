//https://github.com/yknishidate/single-file-vulkan-pathtracing
//https://nvpro-samples.github.io/vk_raytracing_tutorial_KHR/

#version 460
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_ray_tracing : require

#define CLOSEST_HIT_PBR

#include "../common/common.glsl"
#include "../common/math_constants.glsl"
#include "pcs/pcs_raygen.glsl"
#include "ris.glsl"

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


    vec3 brdf = vec3(0.0);
    vec4 nextSample = samplePbr(gl_WorldRayDirectionEXT,params,seed,brdf);

    float cos_theta_i = max(dot(nextSample.xyz, params.normal), 0.0f);

    payload.hitPosition = posWS;
    payload.hitEmission = material.emissionMetallic.rgb;
    payload.hit = true;
    payload.nextSample = nextSample;
    payload.hitNormal =  params.normal;

    if (nextSample.w > 0.0f)
        payload.weightFactor = brdf * cos_theta_i / nextSample.w;

    if(!hitLight(payload)){
         payload.directContribution = bool(pcs.ris & (1 << 31))
            ? calculateDirectRIS(params,posWS,gl_WorldRayDirectionEXT,MAT_PBR,seed)  
            : calculateDirect(params,posWS,gl_WorldRayDirectionEXT,MAT_PBR,seed);
    }

    payload.seed = seed;
    payload.mirror = false;
}

#undef CLOSEST_HIT_PBR
