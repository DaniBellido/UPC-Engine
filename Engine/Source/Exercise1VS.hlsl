float4 main(float3 pos : MY_POSITION) : SV_POSITION
{
    return float4(pos, 1.0);
}