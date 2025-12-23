#pragma once

namespace tinygltf { class Model; struct Material; }

struct MaterialData {
    Vector4 baseColour;
    BOOL hasColourTexture;  
    UINT padding[3];
};

class BasicMaterial
{
public:
    BasicMaterial() = default;

    // Material Constant Buffer (CBV)
    ID3D12Resource* materialBuffer = nullptr;

    // SRV 
    UINT colourTexSRV = UINT_MAX;        


    // Material data
    Vector4 baseColour = { 1,1,1,1 };
    BOOL hasColourTexture = FALSE;

    void load(const tinygltf::Model& model, const tinygltf::Material& material, const char* basePath);
};

