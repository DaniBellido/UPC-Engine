struct PSInput
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD;
    float4 color : COLOR; 
};

float4 main(PSInput input) : SV_TARGET
{
    return input.color;
}