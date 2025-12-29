cbuffer Material : register(b0)
{
    float4 baseColor;
};

Texture2D baseTexture : register(t0);
SamplerState samplerState : register(s0);

struct PSInput
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD;
};

float4 main(PSInput input) : SV_TARGET
{
    float4 texColor = baseTexture.Sample(samplerState, input.texCoord);
    return texColor * baseColor;
}