cbuffer MVP : register(b0)
{
    float4x4 mvpMatrix;
};

// PerInstance CBV (slot 1)
cbuffer PerInstance : register(b1)
{
    float4x4 modelMat;
    float4x4 normalMat;
    float4 dummy0;
    float4 dummy1;
};

struct VSInput
{
    float3 position : POSITION;
    float2 texCoord : TEXCOORD;
    float3 normal : NORMAL;
};

struct VSOutput
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD;
    float3 worldPos : POSITION;
    float3 normal : NORMAL;
};

VSOutput main(VSInput input)
{
    VSOutput output;

    float4 worldPos = mul(float4(input.position, 1.0f), modelMat);
    output.worldPos = worldPos.xyz;

    output.normal = normalize(mul(input.normal, (float3x3) normalMat));

    output.position = mul(float4(input.position, 1.0f), mvpMatrix);
    output.texCoord = input.texCoord;

    return output;
}