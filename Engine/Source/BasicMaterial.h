#pragma once

namespace tinygltf { class Model; struct Material; }

struct BasicMaterialData 
{
    XMFLOAT4 baseColour;
    BOOL hasColourTexture;  
    UINT padding[3];
};

struct PhongMaterialData
{
    XMFLOAT4 diffuseColour;
    float    Kd;
    float    Ks;
    float    shininess;
    BOOL     hasDiffuseTex;
};

struct PBRPhongMaterialData
{
    XMFLOAT3 diffuseColour;
    BOOL     hasDiffuseTex;

    XMFLOAT3 specularColour;
    float    shininess;
};


class BasicMaterial
{
private:

    ComPtr<ID3D12Resource> materialBuffer;
    ComPtr<ID3D12Resource> tex;

    UINT colourTexSRV = UINT_MAX;

    Vector4 baseColour = { 1,1,1,1 };
    BOOL hasColourTexture = FALSE;

public:
    BasicMaterial() = default;

    void load(const tinygltf::Model& model, const tinygltf::Material& material, const char* basePath);

    ID3D12Resource* getMaterialBuffer() const { return materialBuffer.Get(); }
    D3D12_GPU_VIRTUAL_ADDRESS getMaterialBufferGPU() const { return materialBuffer ? materialBuffer->GetGPUVirtualAddress() : 0; }

    UINT  getColourTexSRV()      const { return colourTexSRV; }
    bool  hasTexture()           const { return hasColourTexture == TRUE; }
    const Vector4& getBaseColour() const { return baseColour; }

};

