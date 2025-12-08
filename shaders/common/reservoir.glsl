#ifndef RESERVOIR_GLSL
#define RESERVOIR_GLSL

#include "rng.glsl"

struct CandidateSample{
    // sample direction OR sample hit point
    // for any sample that is not an env map sample, the omega_i variable represents the hit point that the sample hit 
	vec3 omega_i;

    // 1.0 / pdf
    float W;

    // emission of hit surface
    vec3 L_i; 

    // mis weight for this sample
    // if the first bit is positive (the number is negative), the omega_i variable represents hit position
	float misWeight;


    // normal at sample hit point
    // is valid only when omega_i represents position
    vec3 normal; 
};

float unpackMisWeight(float misWeight, inout bool isPosition){
    // leftmost bit mask (sign is stored there)
    uint firstBitMask = 1u << 31; 

    // convert float bits to uint
    uint weightAsUint = floatBitsToUint(misWeight); 

    // if the sign bit is set to 1, the sample's omega_i represents position 
    isPosition = (weightAsUint & firstBitMask) > 0; 

    // zero the sign bit and return the mis weight
    return uintBitsToFloat(weightAsUint & (~firstBitMask)); 
}

float unpackMisWeight(float misWeight){
    // leftmost bit mask (sign is stored there)
    uint firstBitMask = 1u << 31; 

    // convert float bits to uint
    uint weightAsUint = floatBitsToUint(misWeight); 

    // zero the sign bit and return the mis weight
    return uintBitsToFloat(weightAsUint & (~firstBitMask)); 
}

float packMisWeight(float misWeight, bool isPosition){
    // leftmost bit mask (sign is stored there)
    uint firstBitMask = 1 << 31;

    // convert float bits to uint
    uint weightAsUint = floatBitsToUint(misWeight);

    // if the sample represents position, make the sign bit 1, otherwise keep it zero
    return uintBitsToFloat(weightAsUint | (firstBitMask * uint(isPosition)));
}


CandidateSample makeEmptyCandidate(){
    CandidateSample candidate;

    candidate.omega_i = vec3(0.0);
    candidate.normal = vec3(0.0);
    candidate.L_i = vec3(0.0);
    candidate.W = 0.0;
    candidate.misWeight = 0.0f;

    return candidate;
}

struct Reservoir{
    CandidateSample bestSample;
    float wSum;
    float W;
};

Reservoir makeEmptyReservoir(){
    Reservoir r;
    r.bestSample = makeEmptyCandidate();
    r.wSum = 0.0;
    r.W = 0.0;

    return r;
}

bool addSample(inout Reservoir reservoir, CandidateSample candidate, float w, inout uint seed){
    reservoir.wSum += w;

    // do not divide by zero
    if (reservoir.wSum <= 0.0)
        return false;

    if (rand(seed) < w / reservoir.wSum){
        reservoir.bestSample = candidate;
        return true;
    }
    return false;
}

#endif // RESERVOIR_GLSL