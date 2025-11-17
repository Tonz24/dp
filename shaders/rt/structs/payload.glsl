#ifndef PAYLOAD_GLSL
#define PAYLOAD_GLSL


struct HitPayload{
    vec3 hitPosition;
    vec3 hitEmission;
    vec3 directContribution;
    vec3 weightFactor;
    vec3 hitNormal;
    vec4 nextSample;
    uint seed;
    bool hit;
    bool mirror;
};

//  resets payload to default values
void resetPayload(inout HitPayload payload){
    payload.hitPosition = vec3(0.0);
    payload.hitEmission = vec3(0.0);
    payload.weightFactor = vec3(0.0);
    payload.nextSample = vec4(0.0);
    payload.hitNormal = vec3(0.0);
    payload.hit = false;
    payload.directContribution = vec3(0.0);
    payload.seed = 0;
    payload.mirror = false;
}

bool hitLight(HitPayload payload){
    return any(greaterThan(payload.hitEmission,vec3(0.0)));
}

bool hitLight(vec3 emission){
    return any(greaterThan(emission,vec3(0.0)));
}

struct BRDFSamplePayload{
    vec3 hitPosition;
    vec3 hitEmission;
    vec3 hitNormal;
    vec3 throughput;
    float hitArea;
    bool didHit;
    bool mirror;
};

struct TracedSample{
    vec3 hitPosition;
    vec3 hitEmission;
    vec3 hitNormal;
    float hitArea;
    bool didHit;
    bool mirror;
};

TracedSample makeTraced(BRDFSamplePayload payload){
    TracedSample s;

    s.hitPosition = payload.hitPosition;
    s.hitEmission = payload.hitEmission;
    s.hitNormal = payload.hitNormal;
    s.hitArea = payload.hitArea;
    s.didHit = payload.didHit;
    s.mirror = payload.mirror;

    return s;
}

void resetBRDFSamplePayload(inout BRDFSamplePayload payload){
    payload.hitPosition = vec3(0.0);
    payload.hitEmission = vec3(0.0);
    payload.hitNormal = vec3(0.0);
    payload.hitArea = 0.0;
    payload.didHit = false;
    payload.mirror = false;
}

bool hitLight(TracedSample payload){
    return any(greaterThan(payload.hitEmission,vec3(0.0)));
}

struct HitPayloadNaive{
    vec3 hitPosition;
    vec3 hitEmission;
    vec3 weightFactor;
    vec3 hitNormal;
    vec4 nextSample;
    uint seed;
    bool hit;
};

void resetPayload(inout HitPayloadNaive payload){
    payload.hitPosition = vec3(0.0);
    payload.hitEmission = vec3(0.0);
    payload.weightFactor = vec3(0.0);
    payload.nextSample = vec4(0.0);
    payload.hitNormal = vec3(0.0);
    payload.seed = 0;
}

#endif // PAYLOAD_GLSL