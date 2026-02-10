#ifndef PCS_RAYGEN_GLSL
#define PCS_RAYGEN_GLSL

#include "../../common/common.glsl"


layout(push_constant, std140) uniform PushConstants {
    uint albedoMapHandle;
    uint normalMapHandle;
    uint depthMapHandle;
    uint materialMapHandle;

    uint targetHandle;
    uint skyHandle;
    uint skyCdfHandle;

    uint seed;
    uint accumulate;
    uint frameCtr;

    uint maxRecursionDepth;

    uint ris;
    float neighborSearchRadius;
} pcs;


const uint risShift = 31;

const uint brdfShift = 0 * 6;
const uint areaShift = 1 * 6;
const uint envShift =  2 * 6;
const uint neighborShift =  3 * 6;
const uint bufferIndexShift =  4 * 6;

const uint brdfMask = 0x3F << brdfShift;
const uint areaMask = 0x3F << areaShift;
const uint envMask = 0x3F << envShift;
const uint neighborMask = 0x3F << neighborShift;
const uint bufferIndexMask = 0x01 << bufferIndexShift;

bool doRIS(){
    return bool(pcs.ris & (1 << risShift));
}

uint getBrdfSampleCount(){
    return (pcs.ris & brdfMask) >> brdfShift;
}

uint getAreaSampleCount(){
    return (pcs.ris & areaMask) >> areaShift;
}

uint getEnvSampleCount(){
    return (pcs.ris & envMask) >> envShift;
}

uint getNeighborCount(){
    return (pcs.ris & neighborMask) >> neighborShift;
}

uint getBufferReadIndex(){
    return uint(abs(int((pcs.ris & bufferIndexMask) >> bufferIndexShift) - 1));
}

uint getBufferWriteIndex(){
    return (pcs.ris & bufferIndexMask) >> bufferIndexShift;
}


bool unpackFirstHit(vec2 uv, inout ShadeParams params, inout vec3 posWS, inout vec3 rayDir, inout Material mat){
    //  if there's no geometry, draw the sky into the output image and early exit (no rays traced)
    //  (in this case, the sky texture is already pre-rendered in the G buffer shading target texture) 
    vec4 normalRoughness = texture(textures[pcs.normalMapHandle], uv);
    if (!isGeometryValid(normalRoughness.xyz)){
        vec3 sky = texture(textures[pcs.targetHandle],uv).xyz;
        imageStore(outputImage, ivec2(gl_LaunchIDEXT.xy), vec4(sky, 1.0));
        return false;
    }

    // early exit on emissive materials -- write emission directly into output image
    mat = materialUBO.materials[uint(texture(utextures[pcs.materialMapHandle],uv).r)];
    if (any(notEqual(mat.emissionMetallic.rgb,vec3(0.0)))){
        imageStore(outputImage, ivec2(gl_LaunchIDEXT.xy), vec4(mat.emissionMetallic.rgb, 1.0));
        return false;
    }

    //  load the rest of the textures
    params.normal = normalize(normalRoughness.xyz);
    if (dot(-rayDir,params.normal) < 0.0 )
        params.normal *= -1.0f;

    params.roughness = normalRoughness.w;

    vec4 albedoMetallic = texture(textures[pcs.albedoMapHandle], uv);
    params.albedo = albedoMetallic.rgb;
    params.metallic = albedoMetallic.w;

    float depth = texture(textures[pcs.depthMapHandle], uv).x;

    // initial ray and shading parameters
    // at first, take them from the G buffer fill step, then reuse them when bouncing rays  
    posWS = reconstructPositionWS(uv,depth);
    rayDir = normalize(posWS - cameraUBO.posWS);

    return true;
}

bool unpackFirstHitRestir(vec2 uv, inout ShadeParams params, inout vec3 posWS, inout vec3 rayDir, inout Material mat){
    //  if there's no geometry, draw the sky into the output image and early exit (no rays traced)
    //  (in this case, the sky texture is already pre-rendered in the G buffer shading target texture) 
    vec4 normalRoughness = texture(textures[pcs.normalMapHandle], uv);
    mat = materialUBO.materials[uint(texture(utextures[pcs.materialMapHandle],uv).r)];

    // early exit on emissive materials or invalid normals
    if (any(notEqual(mat.emissionMetallic.rgb,vec3(0.0))) || !isGeometryValid(normalRoughness.xyz))
        return false;

    //  load the rest of the textures
    params.normal = normalize(normalRoughness.xyz);
    if (dot(-rayDir,params.normal) < 0.0 )
        params.normal *= -1.0f;

    params.roughness = normalRoughness.w;

    vec4 albedoMetallic = texture(textures[pcs.albedoMapHandle], uv);
    params.albedo = albedoMetallic.rgb;
    params.metallic = albedoMetallic.w;

    float depth = texture(textures[pcs.depthMapHandle], uv).x;

    // initial ray and shading parameters
    // at first, take them from the G buffer fill step, then reuse them when bouncing rays  
    posWS = reconstructPositionWS(uv,depth);
    rayDir = normalize(posWS - cameraUBO.posWS);

    return true;
}

#endif // PCS_RAYGEN_GLSL
