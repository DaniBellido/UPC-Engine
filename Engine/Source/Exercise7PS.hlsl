static const float PI = 3.14159265f;

// ------------------------------------------------------------
// PerFrame CBV (b2)
// ------------------------------------------------------------
cbuffer PerFrame : register(b2)
{
    float3 L;
    float pad0;
    float3 Lc;
    float pad1;
    float3 Ac;
    float pad2;
    float3 viewPos;
    float pad3;
};

// ------------------------------------------------------------
// PerInstance CBV (b1) — PBR-Phong
// ------------------------------------------------------------
cbuffer PerInstance : register(b1)
{
    float4x4 modelMat;
    float4x4 normalMat;

    float3 diffuseColour; // Albedo
    bool hasDiffuseTex;

    float3 specularColour; // F0
    float shininess;
};

// ------------------------------------------------------------
Texture2D baseTexture : register(t0);
SamplerState samplerState : register(s0);

// ------------------------------------------------------------
struct PSInput
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD;
    float3 worldPos : POSITION;
    float3 normal : NORMAL;
};

// ------------------------------------------------------------
// Fresnel Schlick
// ------------------------------------------------------------
float3 FresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0f - F0) * pow(1.0f - cosTheta, 5.0f);
}

// ------------------------------------------------------------
float4 main(PSInput input) : SV_TARGET
{
    float3 N = normalize(input.normal);
    float3 Ldir = normalize(-L);
    float3 V = normalize(viewPos - input.worldPos);
    float3 H = normalize(Ldir + V);

    float NdotL = saturate(dot(N, Ldir));
    float NdotV = saturate(dot(N, V));
    float NdotH = saturate(dot(N, H));
    float VdotH = saturate(dot(V, H));

    // -----------------------------
    // Base color (albedo)
    // -----------------------------
    float3 albedo = diffuseColour;
    if (hasDiffuseTex)
    {
        albedo *= baseTexture.Sample(samplerState, input.texCoord).rgb;
    }

    // -----------------------------
    // Fresnel
    // -----------------------------
    float3 F = FresnelSchlick(VdotH, specularColour);

    // -----------------------------
    // Diffuse (Lambert normalized)
    // -----------------------------
    float3 diffuse = albedo / PI;

    // -----------------------------
    // Specular (Phong normalized)
    // -----------------------------
    float phongNorm = (shininess + 2.0f) / (2.0f * PI);
    float3 specular = phongNorm * pow(NdotH, shininess) * F;

    // -----------------------------
    // Energy conservation
    // -----------------------------
    float3 kd = 1.0f - F;

    // -----------------------------
    // Lighting
    // -----------------------------
    float3 color =
        Ac * albedo +
        (kd * diffuse + specular) * Lc * NdotL;

    return float4(color, 1.0f);
}