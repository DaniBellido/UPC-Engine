static const float PI = 3.14159265f;
static const float GAMMA = 2.2f;
static const float INV_GAMMA = 1.0f / GAMMA;

// -------------------------------
// Light structs
// -------------------------------
struct DirectionalLight
{
    float3 direction;
    float3 color;
    float intensity;
};

struct PointLight
{
    float3 position;
    float3 color;
    float intensity;
    float radius;
};

struct SpotLight
{
    float3 position;
    float3 direction;
    float3 color;
    float intensity;
    float radius;
    float cosInnerAngle;
    float cosOuterAngle;
    float pad0;
};

// -------------------------------
// StructuredBuffers (arrays)
// -------------------------------
StructuredBuffer<DirectionalLight> DirLights : register(t0);
StructuredBuffer<PointLight> PointLights : register(t1);
StructuredBuffer<SpotLight> SpotLights : register(t2);

// -------------------------------
// PerFrame (b2): globals + counts
// -------------------------------
cbuffer PerFrame : register(b2)
{
    float3 Ac;
    float pad0;

    float3 viewPos;
    float pad1;

    uint NumDirLights;
    uint NumPointLights;
    uint NumSpotLights;
    uint pad2;
};

// -------------------------------
// PerInstance (b1): transforms + material
// -------------------------------
cbuffer PerInstance : register(b1)
{
    float4x4 modelMat;
    float4x4 normalMat;

    float3 diffuseColour;
    bool hasDiffuseTex;

    float3 specularColour;
    float shininess;
};

// -------------------------------
// Texture moved to t3
// -------------------------------
Texture2D baseTexture : register(t3);
SamplerState samplerState : register(s0);

struct PSInput
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD;
    float3 worldPos : POSITION;
    float3 normal : NORMAL;
};

float3 LinearToSRGB(float3 c)
{
    return pow(saturate(c), INV_GAMMA);
}

float3 Schlick(float3 F0, float cosTheta)
{
    return F0 + (1.0f - F0) * pow(1.0f - cosTheta, 5.0f);
}

float EpicAttenuation(float dist, float radius)
{
    float r = max(radius, 1e-4f);
    float d = dist / r;
    float d2 = d * d;
    float d4 = d2 * d2;

    float falloff = saturate(1.0f - d4);
    falloff *= falloff;

    return falloff / (dist * dist + 1.0f);
}

float Smoothstep(float e0, float e1, float x)
{
    float t = saturate((x - e0) / max(e1 - e0, 1e-4f));
    return t * t * (3.0f - 2.0f * t);
}

float3 PBRNeutralToneMapping(float3 color)
{
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

float3 EvaluateBRDF(float3 Cd, float3 Cs, float sh, float3 N, float3 V, float3 Lrays, out float NdotL)
{
    float3 toLight = -Lrays;

    NdotL = saturate(dot(N, toLight));
    if (NdotL <= 0.0f)
        return 0.0f;

    float phongNorm = (sh + 2.0f) / (2.0f * PI);

    float3 R = reflect(Lrays, N);
    float VdotR = saturate(dot(V, R));

    float3 F = Schlick(Cs, NdotL);

    float rf0Max = max(max(Cs.r, Cs.g), Cs.b);
    float3 diffuse = (Cd * (1.0f - rf0Max)) / PI;
    float3 spec = phongNorm * F * pow(VdotR, sh);

    return diffuse + spec;
}

float3 EvalDir(DirectionalLight l, float3 worldPos, float3 N, float3 V, float3 Cd, float3 Cs, float sh)
{
    // L = -Dirlight
    float3 Lrays = normalize(-l.direction);

    float NdotL;
    float3 brdf = EvaluateBRDF(Cd, Cs, sh, N, V, Lrays, NdotL);

    float3 Li = l.color * l.intensity;
    return brdf * Li * NdotL;
}

float3 EvalPoint(PointLight l, float3 worldPos, float3 N, float3 V, float3 Cd, float3 Cs, float sh)
{
    // L = normalize(possurface - poslight)
    float3 toSurface = worldPos - l.position;
    float dist = length(toSurface);
    float3 Lrays = (dist > 1e-4f) ? (toSurface / dist) : 0.0f;

    float att = EpicAttenuation(dist, l.radius);

    float NdotL;
    float3 brdf = EvaluateBRDF(Cd, Cs, sh, N, V, Lrays, NdotL);

    float3 Li = l.color * l.intensity;
    return brdf * Li * NdotL * att;
}

float3 EvalSpot(SpotLight l, float3 worldPos, float3 N, float3 V, float3 Cd, float3 Cs, float sh)
{
    float3 toSurface = worldPos - l.position;
    float dist = length(toSurface);
    float3 Lrays = (dist > 1e-4f) ? (toSurface / dist) : 0.0f;

    float3 Sd = normalize(l.direction);

    // distance attenuation along the light direction
    float distAxis = dot(toSurface, Sd);
    float axisMask = (distAxis > 0.0f) ? 1.0f : 0.0f;
    float att = EpicAttenuation(distAxis, l.radius) * axisMask;

    float cosAngle = dot(Sd, Lrays);
    float cone = Smoothstep(l.cosOuterAngle, l.cosInnerAngle, cosAngle);

    float NdotL;
    float3 brdf = EvaluateBRDF(Cd, Cs, sh, N, V, Lrays, NdotL);

    float3 Li = l.color * l.intensity;
    return brdf * Li * NdotL * att * cone;
}

float4 main(PSInput input) : SV_TARGET
{
    float3 Cd = diffuseColour;
    if (hasDiffuseTex)
    {
        float3 tex = baseTexture.Sample(samplerState, input.texCoord).rgb;
        Cd *= tex;
    }

    float3 N = normalize(input.normal);
    float3 V = normalize(viewPos - input.worldPos);

    float3 result = Ac * Cd;

    for (uint i = 0; i < NumDirLights; ++i)
        result += EvalDir(DirLights[i], input.worldPos, N, V, Cd, specularColour, shininess);

    for (uint i = 0; i < NumPointLights; ++i)
        result += EvalPoint(PointLights[i], input.worldPos, N, V, Cd, specularColour, shininess);

    for (uint i = 0; i < NumSpotLights; ++i)
        result += EvalSpot(SpotLights[i], input.worldPos, N, V, Cd, specularColour, shininess);

    // tone mapping then gamma as the last step
    result = PBRNeutralToneMapping(result);
    result = LinearToSRGB(result);

    return float4(result, 1.0f);
}
