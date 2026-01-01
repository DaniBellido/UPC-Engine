// PerFrame CBV (slot 2) 
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

cbuffer PerInstance : register(b1)
{
    float4x4 modelMat;
    float4x4 normalMat;
    float4 diffuseColour;
    float Kd;
    float Ks;
    float shininess;
    bool hasDiffuseTex;
};

Texture2D baseTexture : register(t0);
SamplerState samplerState : register(s0);

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
    float3 Ldir = normalize(-L);
    float3 V = normalize(viewPos - input.worldPos);

    // Ambient
    float3 ambient = Ac * diffuseColour.rgb * Kd;
    
    float NdotL = saturate(dot(N, Ldir));

    // Diffuse
    float3 diffuse = Kd * NdotL * Lc * diffuseColour.rgb;

    // Specular
    float3 R = reflect(-Ldir, N);
    float specAngle = saturate(dot(R, V));
    float spec = pow(specAngle, shininess);
    float3 specular = Ks * spec * Lc;
    
    float3 texColor = diffuseColour.rgb;

    // Texture
    if (hasDiffuseTex != 0)
    {
        texColor *= baseTexture.Sample(samplerState, input.texCoord).rgb;
    }

    float3 color = ambient * texColor + diffuse * texColor + specular;

    
    //TESTS
    //return float4(abs(input.normal), 1.0f);
    //return diffuseColour;
    
    return float4(color, diffuseColour.a);
   
}