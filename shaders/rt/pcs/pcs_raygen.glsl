#ifndef PCS_RAYGEN_GLSL
#define PCS_RAYGEN_GLSL


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
} pcs;


const uint risShift = 31;

const uint brdfShift = 0 * 6;
const uint areaShift = 1 * 6;
const uint envShift =  2 * 6;

const uint brdfMask = 0x3F << brdfShift;
const uint areaMask = 0x3F << areaShift;
const uint envMask = 0x3F << envShift;

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

#endif // PCS_RAYGEN_GLSL
