#ifndef PBR_GLSL
#define PBR_GLSL

vec3 fresnelSchlick(vec3 F_0, float cos_theta_h){
    float v = 1.0 - cos_theta_h;
    v = v * v * v * v * v; //fifth power
    return F_0 + (1.0 - F_0) * v;
}

float distrGGX(float alpha, float cos_theta_m){

    if (cos_theta_m <= 0)
        return 0.0f;

    alpha = max(alpha, 1e-3);
    cos_theta_m = clamp(cos_theta_m, 0.0f, 1.0f);

    float a2 = alpha * alpha;
    float cos_theta_m2 = cos_theta_m * cos_theta_m;

    float term = (a2 - 1.0f) * cos_theta_m2 + 1;
    float term2 = term * term;

    float denom = PI * term2;

    float res = a2 / denom; 

    if (isinf(res) || isnan(res))
        return 0.0f;

    return res;
}

float auxSmith(float cos_theta, float alpha){
    float a2 = alpha * alpha;
    float cos_theta2 = cos_theta * cos_theta;
    float term =  sqrt(1.0 + a2 * (1.0 / cos_theta2 - 1.0));

    return (term - 1.0) / 2.0;
}

float G_Smith(float cos_theta_o, float cos_theta_i, float alpha){
    float aux_theta_o = auxSmith(cos_theta_o, alpha);
    float aux_theta_i = auxSmith(cos_theta_i, alpha);

    return 1.0 / (1.0 + aux_theta_o + aux_theta_i);
}

// https://auzaiffe.wordpress.com/2024/04/15/vndf-importance-sampling-an-isotropic-distribution/
// https://gist.github.com/jdupuy/4c6e782b62c92b9cb3d13fbb0a5bd7a0#file-samplevndf_ggx-cpp-L51
vec3 SampleVndf_GGX(vec2 u, vec3 wi, float alpha, vec3 n) {
    
    // Dirac function for alpha = 0
    if (alpha == 0.0f) return n;
    // decompose the vector in parallel and perpendicular components
    vec3 wi_z = n * dot(wi, n);
    vec3 wi_xy = wi - wi_z;
    // warp to the hemisphere configuration
    vec3 wiStd = normalize(wi_z - alpha * wi_xy);
    // sample a spherical cap in (-wiStd.z, 1]
    float wiStd_z = dot(wiStd, n);
    float phi = (2.0f * u.x - 1.0f) * PI;
    float z = (1.0f - u.y) * (1.0f + wiStd_z) - wiStd_z;
    float sinTheta = sqrt(clamp(1.0f - z * z, 0.0f, 1.0f));
    float x = sinTheta * cos(phi);
    float y = sinTheta * sin(phi);
    vec3 cStd = vec3(x, y, z);
    // reflect sample to align with normal
    vec3 up = vec3(0, 0, 1);
    vec3 wr = n + up;
    // prevent division by zero
    float wrz_safe = max(wr.z, 1e-6);
    vec3 c = dot(wr, cStd) * wr / wrz_safe - cStd;
    // compute halfway direction as standard normal
    vec3 wmStd = c + wiStd;
    vec3 wmStd_z = n * dot(n, wmStd);
    vec3 wmStd_xy = wmStd_z - wmStd;
    // warp back to the ellipsoid configuration
    vec3 wm = normalize(wmStd_z + alpha * wmStd_xy);
    // return final normal
    return wm;
}


// https://auzaiffe.wordpress.com/2024/04/15/vndf-importance-sampling-an-isotropic-distribution/
float pdf_vndf_isotropic(vec3 wo, vec3 wi, float alpha, vec3 n) {

    float alphaSquare = alpha * alpha;
    vec3 wm = normalize(wo + wi);
    float zm = dot(wm, n);
    float zi = dot(wi, n);
    float nrm = inversesqrt((zi * zi) * (1.0f - alphaSquare) + alphaSquare);
    float sigmaStd = (zi * nrm) * 0.5f + 0.5f;
    float sigmaI = sigmaStd / nrm;
    float nrmN = (zm * zm) * (alphaSquare - 1.0f) + 1.0f;
    return alphaSquare / (PI * 4.0f * nrmN * nrmN * sigmaI);
}

#endif // PBR_GLSL