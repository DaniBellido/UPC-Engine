Texture2D colourTex : register(t0);
SamplerState colourSampler : register(s0);
float4 main(float2 coords : TEXCOORD) : SV_TARGET
{
    return colourTex.Sample(colourSampler, coords); // sets the pixel colour from the texture sampled
}