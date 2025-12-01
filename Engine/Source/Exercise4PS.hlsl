struct PSInput
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD;
    float4 color : COLOR; // Recibir color del VS
};

float4 main(PSInput input) : SV_TARGET
{
    return input.color; // Devolver el color interpolado (colores por cara)
}