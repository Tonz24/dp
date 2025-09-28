struct HitPayload{
    vec3 hitPosition;
    vec3 hitNormal;
    vec3 hitEmission;
    vec3 hitBrdf;
    vec3 albedo;
    bool hit;
};

//  resets payload to default values
void resetPayload(inout HitPayload payload){
    payload.hitPosition = vec3(0.0);
    payload.hitNormal = vec3(0.0);
    payload.hitEmission = vec3(0.0);
    payload.hitBrdf = vec3(0.0);
    payload.hit = false;
    payload.albedo = vec3(0.0);
}