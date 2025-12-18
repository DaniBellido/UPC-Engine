cbuffer Transforms : register(b0)
{
    float4x4 mvp;
};
struct VertexOutput
{
    float2 texCoord : TEXCOORD;
    float4 position : SV_POSITION;
};
VertexOutput main(float3 position : POSITION, float2 texCoord : TEXCOORD)
{
    VertexOutput output;
    output.position = mul(float4(position, 1.0f), mvp);
    output.texCoord = texCoord;
    return output;
}