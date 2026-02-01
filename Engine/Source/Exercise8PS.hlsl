static const float PI = 3.14159265f;

// ------------------------------------------------------------
// PerFrame CBV (b2)
// ------------------------------------------------------------
cbuffer PerFrame : register(b2)
{
    // Directional light
    float3 L;
    float pad0;
    
    // Directional light color * intensity
    float3 Lc;
    float pad1;
    
    // Ambient light color
    float3 Ac;
    float pad2;
    
     // Camera position in world space
    float3 viewPos;
    float pad3;
    
    // Point light parameters
    float3 pointPos; // point light position in world space
    float pointRange; // max distance of influence
    float3 pointColor; // point light color
    float pointIntensity; // point light intensity multiplier
};

// ------------------------------------------------------------
// PerInstance CBV (b1)
// ------------------------------------------------------------
cbuffer PerInstance : register(b1)
{
    float4x4 modelMat;
    float4x4 normalMat;

    float3 diffuseColour;
    bool hasDiffuseTex;

    float3 specularColour;
    float shininess;
};

// ------------------------------------------------------------
// Texture + sampler
// ------------------------------------------------------------
Texture2D baseTexture : register(t0);
SamplerState samplerState : register(s0);

// ------------------------------------------------------------
// Pixel shader input (from VS)
// worldPos and normal MUST be in world space for this to work correctly
// ------------------------------------------------------------
struct PSInput
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD;
    float3 worldPos : POSITION;
    float3 normal : NORMAL;
};

// ------------------------------------------------------------
// Fresnel Schlick approximation
// F0: base reflectance (specularColour)
// cosTheta: some cosine term (here we use dotNL / dotNLp, matching your style)
// ------------------------------------------------------------
float3 Schlick(float3 F0, float cosTheta)
{
    return F0 + (1.0f - F0) * pow(1.0f - cosTheta, 5.0f);
}

// ------------------------------------------------------------
float4 main(PSInput input) : SV_TARGET
{
    
    // =========================================================================
    // 1) Material albedo (diffuse) color Cd
    // =========================================================================
    float3 Cd = diffuseColour;
    
    // If we have a diffuse texture, multiply the material color by the texture
    if (hasDiffuseTex)
    {
        Cd *= baseTexture.Sample(samplerState, input.texCoord).rgb;
    }

    // =========================================================================
    // 2) Basic vectors: Normal (N) and View direction (V)
    // =========================================================================
    float3 N = normalize(input.normal);
    float3 V = normalize(viewPos - input.worldPos);
    
    // This takes the maximum channel of the specular color
    // You use it to reduce diffuse energy when specular is strong
    float rf0Max = max(max(specularColour.r, specularColour.g), specularColour.b);
    
    // ============================================================
    // A) DIRECTIONAL LIGHT
    // ============================================================
    float3 R = reflect(L, N);
    
    // dot(V, R) tells us how aligned the camera is with the reflection direction
    float dotVR = saturate(dot(V, R));
    
    // Lambert term
    // Because L is "light rays" (light -> surface), the "to light" direction is -L
    // So dot(N, toLight) == dot(N, -L) == -dot(L, N)
    float dotNL = saturate(-dot(L, N));

    // Fresnel uses dotNL as cosTheta
    float3 fresnel = Schlick(specularColour, dotNL);

    // BRDF combination
    float3 brdfD =
        (Cd * (1.0f - rf0Max)) / PI
        + ((shininess + 2.0f) / 2.0f * PI) * fresnel * pow(dotVR, shininess);
    
    // Final directional contribution = BRDF * lightColor * lambert
    float3 dirTerm = brdfD * Lc * dotNL;
    // =========================================================================
    // B) POINT LIGHT contribution
    // =========================================================================

    // We keep the SAME convention as directional:
    // Lp will be the direction of LIGHT RAYS (point light -> surface point).
    // Ray direction from point light to surface:
    float3 pointToSurface = input.worldPos - pointPos;

    // Distance from point light to surface point
    float dist = length(pointToSurface);

    // Prevent divide-by-zero
    float invDist = (dist > 1e-4f) ? (1.0f / dist) : 0.0f;

    // Lp = normalized ray direction (light -> surface)
    float3 Lp = pointToSurface * invDist;

    // Simple range-based attenuation:
    // - 1 at the light position
    // - 0 at pointRange (and beyond)
    // Squared for a smoother falloff
    float safeRange = max(pointRange, 1e-4f);
    float atten = saturate(1.0f - dist / safeRange);
    atten *= atten;

    // Lambert for point light with our "rays" convention
    float dotNLp = saturate(-dot(Lp, N));

    // Specular (Phong) for point light:
    // reflect expects incident direction (incoming rays), so we use Lp directly
    float3 Rp = reflect(Lp, N);
    float dotVRp = saturate(dot(V, Rp));

    // Point light color/intensity
    float3 pointLc = pointColor * pointIntensity;

    // Fresnel for point light (same style as directional)
    float3 fresnelP = Schlick(specularColour, dotNLp);

    // Same BRDF model, but using point light angles
    float3 brdfP =
        (Cd * (1.0f - rf0Max)) / PI
        + ((shininess + 2.0f) / 2.0f * PI) * fresnelP * pow(dotVRp, shininess);

    // Final point light contribution includes attenuation
    float3 pointTerm = brdfP * pointLc * dotNLp * atten;

    // =========================================================================
    // C) AMBIENT contribution
    // =========================================================================
    float3 ambientTerm = Ac * Cd;

    // =========================================================================
    // Final color = sum of all light terms
    // =========================================================================
    float3 colour = dirTerm + pointTerm + ambientTerm;

    return float4(colour, 1.0f);
}
