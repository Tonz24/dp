struct HitPayload{
    vec3 hitPosition;
    vec3 hitEmission;
    vec3 directContribution;
    vec3 weightFactor;
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
    payload.hit = false;
    payload.directContribution = vec3(0.0);
    payload.seed = 0;
    payload.mirror = false;
}

bool hitLight(HitPayload payload){
    return any(greaterThan(payload.hitEmission,vec3(0.0)));
}