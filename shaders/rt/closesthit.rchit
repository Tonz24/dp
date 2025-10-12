//https://github.com/yknishidate/single-file-vulkan-pathtracing
//https://nvpro-samples.github.io/vk_raytracing_tutorial_KHR/

#version 460
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_ray_tracing : require

#include "../common/common.glsl"
#include "../common/math_constants.glsl"
#include "pcs/pcs_raygen.glsl"
#include "raycommon.glsl"
#include "structs/payload.glsl"

layout(location = 0) rayPayloadInEXT HitPayload payload;
layout(location = 1) rayPayloadEXT BRDFSamplePayload payloadBRDF;

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
    mat3 N = transpose(inverse(mat3(gl_ObjectToWorldEXT)));
    return (N * tangent);
}

vec2 getTexCoord(vec3 uv, Vertex v0, Vertex v1, Vertex v2){
    vec2 texCoord = uv.x * v0.texCoord + uv.y * v1.texCoord + uv.z * v2.texCoord;
    return texCoord;
}

TracedSample traceBRDFLighting(vec3 posWS, vec3 direction){
    float tMin = 0.001f;
    float tMax = 10000.0f;

    resetBRDFSamplePayload(payloadBRDF);
    traceRayEXT(
            topLevelAS,
            gl_RayFlagsOpaqueEXT,
            0xff, // cullMask
            2,    // sbtRecordOffset
            0,    // sbtRecordStride
            0,    // missIndex
            posWS,
            tMin,
            direction,
            tMax,
            1     // brdf sample always has payload location 1
    );

    return makeTraced(payloadBRDF);
}

float powerHeuristic(float pdf, float pdfOther) {
	const float pdf_sqr = pdf * pdf;
	const float pdfOther_sqr = pdfOther * pdfOther;
	const float result = pdf_sqr / (pdfOther_sqr + pdf_sqr);
	return result;
}


// evaluates direct light sample, weighs it by MIS weight
vec3 evaluateLightSample(vec3 albedo, vec3 posWS, vec3 normal, inout uint seed){
    // take CDF sample (pick an emissive triangle with probability proportional to the area of all emissive triangles)
    uint cdfSampleIndex = sampleCDFIndex(rand(seed));
    CDFElement cdfSample = emissiveCDF.cdf[cdfSampleIndex];

    TrianglePacked cdfTriangle = emissiveBuffer.tris[cdfSample.triIndex];
    vec3 L_i = vec3(cdfTriangle.v0eR.w,cdfTriangle.v1eG.w,cdfTriangle.v2eB.w);

    // sample the triangle identified by CDF sample (pick a random point on the triangle with uniform probability)
    TriangleSample sampledPoint = sampleTriangle(cdfTriangle,seed);

    // early exit if occlusion test fails as there wouldn't be any light contribution anyway
    if (!isVisible(posWS, sampledPoint.position)) return vec3(0.0);

    float pdfCDF = cdfSampleIndex == 0 ? cdfSample.cdfVal : cdfSample.cdfVal - emissiveCDF.cdf[cdfSampleIndex-1].cdfVal;
    float lightPdf = sampledPoint.pdf * pdfCDF;

    vec3 omega_i = sampledPoint.position - posWS;
    float r_sqr = dot(omega_i, omega_i);
    omega_i = normalize(omega_i);

    // early exit when invalid numbers are present
    if (r_sqr == 0.0 || any(isnan(omega_i)) || any(isinf(omega_i)) || lightPdf == 0.0) return vec3(0.0);

    float cos_theta_i = max(dot(omega_i, normal),0.0);
    float cos_theta_y = max(dot(-omega_i, sampledPoint.normal),0.0);

    // any of these being zero means that radiance is also zero
    if (cos_theta_i == 0.0 || cos_theta_y == 0.0) return vec3(0.0);

    float areaMeasureFactor = cos_theta_y / r_sqr;
    // get the pdf of this sample as if it came from BRDF sampling
    float brdfPdf = getPdfHemisphereCosineWeighted(omega_i,normal);
    // convert to area measure
    brdfPdf *= areaMeasureFactor;

    float misWeight = powerHeuristic(lightPdf, brdfPdf);
    if (misWeight <= 0.0) return vec3(0.0);

    vec3 brdf = albedo / PI;
    float G = areaMeasureFactor;
    vec3 L_direct = misWeight * L_i * brdf * G * cos_theta_i / lightPdf;

    return L_direct;
}

vec3 evaluateBRDFSample(vec3 albedo, vec3 posWS, vec3 normal, inout uint seed){

    //  sample the BRDF
    vec4 brdfSample = sampleHemisphereCosineWeighted(normal, seed);
    vec3 omega_i = brdfSample.xyz;
    float brdfPdf = brdfSample.w;

    //  check what the BRDF sample hits 
    TracedSample brdfHit = traceBRDFLighting(posWS, omega_i);

    //  if the BRDF sample hits nothing or it hits a non emissive surface, early exit
    if (!brdfHit.didHit || !hitLight(brdfHit)) return vec3(0.0);

    vec3 L_i = brdfHit.hitEmission;

    vec3 toHit = brdfHit.hitPosition - posWS;
    float r_sqr = dot(toHit, toHit);

    float cos_theta_i = max(dot(omega_i, normal), 0.0f);
    float cos_theta_y = max(dot(-omega_i, brdfHit.hitNormal), 0.0f);

    if (cos_theta_i == 0.0 || cos_theta_y == 0.0) return vec3(0.0);

    float areaMeasureFactor = cos_theta_y / r_sqr;
    float brdfPdfAreaMeasure = brdfPdf * areaMeasureFactor;

    float pdfLight = (1.0 / brdfHit.hitArea) * (brdfHit.hitArea / emissiveCDF.area);
    float misWeight = powerHeuristic(brdfPdfAreaMeasure, pdfLight);

    if (misWeight <= 0.0) return vec3(0.0);

    vec3 brdf = albedo * INVPI;
    vec3 L_direct = misWeight * L_i * brdf * cos_theta_i / brdfPdf; 
    return L_direct;
}

vec3 calculateDirect(vec3 albedo, vec3 posWS, vec3 normal, inout uint seed){
    vec3 directContribution = vec3(0.0);

    directContribution += evaluateLightSample(albedo, posWS, normal, seed);
    directContribution += evaluateBRDFSample(albedo, posWS, normal, seed);

    return directContribution;
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

    payload.weightFactor = hitBrdf * max(dot(nextDir,params.normal),0.0) / pdf;

    if(hitLight(payload) && pcs.NEE == 1)
        return;
    
    if (pcs.NEE == 1){
        payload.directContribution = evaluateDirectLighting(params.albedo,posWS,params.normal,seed);
        //payload.directContribution = calculateDirect(params.albedo,posWS,params.normal,seed);
        //payload.directContribution = sampleBRDF(posWS,params.normal,seed);
    }

    payload.seed = seed;
}