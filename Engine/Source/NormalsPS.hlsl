struct PSInput
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD;
    float3 worldPos : POSITION;
    float3 normal : NORMAL;
};

float4 main(PSInput input) : SV_TARGET
{
    float3 N = normalize(input.normal);
    return float4(abs(N), 1.0);
}