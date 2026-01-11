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
float3 Schlick(float3 F0, float cosTheta)
{
    return F0 + (1.0f - F0) * pow(1.0f - cosTheta, 5.0f);
}

// ------------------------------------------------------------
float4 main(PSInput input) : SV_TARGET
{
    
    float3 Cd = diffuseColour;
    
    if (hasDiffuseTex)
    {
        Cd *= baseTexture.Sample(samplerState, input.texCoord).rgb;
    }

    float3 N = normalize(input.normal);

    
    float3 R = reflect(L, N);
    float3 V = normalize(viewPos - input.worldPos);

    float dotVR = saturate(dot(V, R));
    float dotNL = saturate(-dot(L, N));

    float rf0Max = max(max(specularColour.r, specularColour.g), specularColour.b);
    float3 fresnel = Schlick(specularColour, dotNL);

    float3 colour = ((Cd * (1.0f - rf0Max)) / PI + ((shininess + 2.0f) / 2.0f * PI) * fresnel * pow(dotVR, shininess)) * Lc * dotNL + Ac * Cd;

    return float4(colour, 1.0f);
}
