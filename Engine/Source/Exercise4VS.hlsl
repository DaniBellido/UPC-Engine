cbuffer Transforms : register(b0)
{
    float4x4 mvp;
};

struct VSInput
{
    float3 position : MY_POSITION; // ? Coincide con tu input layout
    float2 texCoord : TEXCOORD; // ? Coincide con tu input layout  
    float4 color : COLOR; // ? NUEVO para colores por cara
};

struct PSInput
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD;
    float4 color : COLOR; // ? Pasar color al pixel shader
};

PSInput main(VSInput input)
{
    PSInput output;
    output.position = mul(float4(input.position, 1.0f), mvp);
    output.texCoord = input.texCoord;
    output.color = input.color; // ? Pasar el color interpolado
    return output;
}