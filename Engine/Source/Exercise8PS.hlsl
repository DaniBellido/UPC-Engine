static const float PI = 3.14159265f;

// ------------------------------------------------------------
// PerFrame CBV (b2)
// ------------------------------------------------------------
cbuffer PerFrame : register(b2)
{
    // Directional light
    float3 L;
    float pad0;

    // Directional light color * intensity
    float3 Lc;
    float pad1;

    // Ambient light color
    float3 Ac;
    float pad2;

    // Camera position in world space
    float3 viewPos;
    float pad3;

    // Point light parameters
    float3 pointPos; // point light position in world space
    float pointRange; // max distance of influence (radius)
    float3 pointColor; // point light color
    float pointIntensity; // point light intensity multiplier

    // Spot light parameters
    // spotDir must be "light rays direction" (light -> scene), normalized
    float3 spotPos; // spot light position in world space
    float spotRange; // max distance of influence (radius along axis)

    float3 spotDir; // spot direction (rays), light -> scene
    float spotIntensity; // intensity multiplier

    float3 spotColor; // spot light color
    float spotInnerCos; // cos(innerAngle). Must be > spotOuterCos

    float spotOuterCos; // cos(outerAngle)
    float padS1;
    float padS2;
    float padS3;
};

// ------------------------------------------------------------
// PerInstance CBV (b1)
// ------------------------------------------------------------
cbuffer PerInstance : register(b1)
{
    float4x4 modelMat;
    float4x4 normalMat;

    float3 diffuseColour;
    bool hasDiffuseTex;

    float3 specularColour;
    float shininess;
};

// ------------------------------------------------------------
// Texture + sampler
// ------------------------------------------------------------
Texture2D baseTexture : register(t0);
SamplerState samplerState : register(s0);

// ------------------------------------------------------------
// Pixel shader input (from VS)
// worldPos and normal MUST be in world space for this to work correctly
// ------------------------------------------------------------
struct PSInput
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD;
    float3 worldPos : POSITION;
    float3 normal : NORMAL;
};

// ============================================================
// Helper functions
// ============================================================

float3 SchlickFresnel(float3 F0, float cosTheta)
{
    // Schlick Fresnel approximation.
    // cosTheta is clamped in caller.
    return F0 + (1.0f - F0) * pow(1.0f - cosTheta, 5.0f);
}

float Smoothstep(float edge0, float edge1, float x)
{
    // Standard smoothstep implementation.
    float t = saturate((x - edge0) / max(edge1 - edge0, 1e-4f));
    return t * t * (3.0f - 2.0f * t);
}

float EpicAttenuation(float dist, float radius)
{
    // "Epic/Unreal-style" smooth radius falloff with an inverse-square-like term.
    // - radius controls the maximum influence.
    // - falloff is 1 near the light and smoothly goes to 0 at radius.
    // - denominator avoids singularities and behaves like inverse-square at mid distances.
    float r = max(radius, 1e-4f);
    float d = dist / r;

    float d2 = d * d;
    float d4 = d2 * d2;

    float falloff = saturate(1.0f - d4);
    falloff *= falloff;

    return falloff / (dist * dist + 1.0f);
}

float3 PBRNeutralToneMapping(float3 color)
{
    // Khronos "PBR Neutral" tone mapper (as used in many teaching materials).
    // This maps HDR values into displayable range while preserving highlights.

    float x = min(color.r, min(color.g, color.b));
    float offset = (x < 0.08f) ? (x - 6.25f * x * x) : 0.04f;
    color -= offset;

    float peak = max(color.r, max(color.g, color.b));
    const float startCompression = 0.8f - 0.04f;

    if (peak < startCompression)
        return color;

    const float d = 1.0f - startCompression;
    float newPeak = 1.0f - d * d / (peak + d - startCompression);
    color *= newPeak / max(peak, 1e-4f);

    const float desaturation = 0.15f;
    float g = 1.0f - 1.0f / (desaturation * (peak - newPeak) + 1.0f);

    return lerp(color, newPeak.xxx, g);
}

float3 LinearToSRGB(float3 linearColor)
{
    // Simple gamma encode (approx sRGB).
    // If your backbuffer is an sRGB RTV and the GPU does sRGB write, you should NOT do this.
    const float invGamma = 1.0f / 2.2f;
    return pow(linearColor, invGamma);
}

float3 EvaluatePhongBRDF(
    float3 Cd,
    float3 Cs,
    float sh,
    float3 N,
    float3 V,
    float3 Lrays,
    out float NdotL)
{
    // We use a consistent convention:
    // - Lrays is the direction of incoming light rays (light -> surface).
    // - The direction to the light source is therefore (-Lrays).
    float3 toLight = -Lrays;

    NdotL = saturate(dot(N, toLight));
    if (NdotL <= 0.0f)
        return 0.0f;

    // Normalized Phong specular.
    float phongNorm = (sh + 2.0f) / (2.0f * PI);

    // Specular reflection direction.
    // reflect(i, n) expects incident vector pointing TOWARDS the surface.
    // Our Lrays (light -> surface) is indeed an incident direction, so it's correct to use it.
    float3 R = reflect(Lrays, N);
    float VdotR = saturate(dot(V, R));

    // Simple energy compensation based on max channel of F0.
    float rf0Max = max(max(Cs.r, Cs.g), Cs.b);

    // Fresnel term.
    float3 F = SchlickFresnel(Cs, NdotL);

    // Lambert diffuse + normalized Phong spec.
    float3 diffuse = (Cd * (1.0f - rf0Max)) / PI;
    float3 spec = phongNorm * F * pow(VdotR, sh);

    return diffuse + spec;
}

// ============================================================
// Main PS
// ============================================================
float4 main(PSInput input) : SV_TARGET
{
    // ============================================================
    // 1) Material albedo (diffuse) color Cd
    // ============================================================
    float3 Cd = diffuseColour;

    // Texture sampling assumes you already read the texture in linear space.
    // If your SRV is NOT sRGB, you'd need an sRGB->linear conversion here.
    if (hasDiffuseTex)
    {
        Cd *= baseTexture.Sample(samplerState, input.texCoord).rgb;
    }

    // ============================================================
    // 2) Shading basis vectors in world space
    // ============================================================
    float3 N = normalize(input.normal);
    float3 V = normalize(viewPos - input.worldPos);

    // ============================================================
    // A) DIRECTIONAL LIGHT
    // ============================================================
    // L is stored as "light rays direction" (light -> surface).
    float3 Ld = normalize(L);

    float NdotLd;
    float3 brdfD = EvaluatePhongBRDF(Cd, specularColour, shininess, N, V, Ld, NdotLd);

    // Lc should already be (color * intensity) on CPU side (per your comment).
    float3 dirTerm = brdfD * Lc * NdotLd;

    // ============================================================
    // B) POINT LIGHT
    // ============================================================
    // pointToSurface is also "rays direction" (light -> surface point).
    float3 pointToSurface = input.worldPos - pointPos;

    float distP = length(pointToSurface);
    float invDistP = (distP > 1e-4f) ? (1.0f / distP) : 0.0f;

    float3 Lp = pointToSurface * invDistP;

    // Epic attenuation with radius (pointRange).
    float attenP = EpicAttenuation(distP, pointRange);

    float NdotLp;
    float3 brdfP = EvaluatePhongBRDF(Cd, specularColour, shininess, N, V, Lp, NdotLp);

    float3 pointLi = pointColor * pointIntensity;
    float3 pointTerm = brdfP * pointLi * NdotLp * attenP;

    // ============================================================
    // C) SPOT LIGHT
    // ============================================================
    // spotToSurface is "rays direction" (light -> surface).
    float3 spotToSurface = input.worldPos - spotPos;

    float distS = length(spotToSurface);
    float invDistS = (distS > 1e-4f) ? (1.0f / distS) : 0.0f;

    float3 Ls = spotToSurface * invDistS;

    // The spotlight axis direction (rays) must be normalized.
    float3 Sd = normalize(spotDir);

    // PDF requirement: distance attenuation along the light axis.
    // Projection of the point onto the spotlight axis (light -> scene).
    float distAxis = dot(spotToSurface, Sd);

    // If the point is behind the spotlight, no contribution.
    float axisMask = (distAxis > 0.0f) ? 1.0f : 0.0f;

    // Range check along axis (spotRange is treated as radius along the axis).
    float rangeMask = (distAxis < spotRange) ? 1.0f : 0.0f;

    // Epic attenuation using axis distance (not Euclidean distance).
    float attenS = EpicAttenuation(distAxis, spotRange) * axisMask * rangeMask;

    // Cone attenuation:
    // cosAngle = 1 when surface direction matches spotlight direction.
    float cosAngle = dot(Sd, Ls);

    // English: Smooth transition between outer and inner cone.
    // Assumes: spotInnerCos > spotOuterCos.
    float spotCone = Smoothstep(spotOuterCos, spotInnerCos, cosAngle);

    float NdotLs;
    float3 brdfS = EvaluatePhongBRDF(Cd, specularColour, shininess, N, V, Ls, NdotLs);

    float3 spotLi = spotColor * spotIntensity;
    float3 spotTerm = brdfS * spotLi * NdotLs * attenS * spotCone;

    // ============================================================
    // D) AMBIENT
    // ============================================================
    float3 ambientTerm = Ac * Cd;

    // ============================================================
    // 3) Sum lighting (HDR) then tone map + gamma
    // ============================================================
    float3 colourHDR = dirTerm + pointTerm + spotTerm + ambientTerm;

    // Tone mapping (HDR -> display range).
    float3 colourLDR = PBRNeutralToneMapping(colourHDR);

    // Clamp after tone mapping.
    colourLDR = saturate(colourLDR);

    // Gamma encode for display (only if backbuffer is NOT sRGB).
    colourLDR = LinearToSRGB(colourLDR);

    return float4(colourLDR, 1.0f);
}
