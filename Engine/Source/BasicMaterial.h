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

public:

    enum class Type
    {
        BASIC,
        PHONG,
        PBR_PHONG
    };

private:

    union MaterialData
    {
        BasicMaterialData basic;
        PhongMaterialData phong;
        PBRPhongMaterialData pbrPhong;
    } materialData;

    Type materialType = Type::BASIC;

    // Buffers
    ComPtr<ID3D12Resource> materialBuffer;           // only basic
    D3D12_GPU_VIRTUAL_ADDRESS materialBufferGPU = 0; // for ring buffer

    ComPtr<ID3D12Resource> tex;
    UINT colourTexSRV = UINT_MAX;

    Vector4 baseColour = { 1,1,1,1 };
    BOOL hasColourTexture = FALSE;

public:
    BasicMaterial() = default;

    void load(const tinygltf::Model& model, const tinygltf::Material& material, Type tyoe, const char* basePath);


    ID3D12Resource* getMaterialBuffer() const { return materialBuffer.Get(); }
    D3D12_GPU_VIRTUAL_ADDRESS getMaterialBufferGPU() const { return materialBufferGPU; }

    UINT  getColourTexSRV()      const { return colourTexSRV; }
    bool  hasTexture()           const { return hasColourTexture == TRUE; }
    const Vector4& getBaseColour() const { return baseColour; }

    // Getters per type
    const BasicMaterialData& getBasicMaterial() const { _ASSERTE(materialType == Type::BASIC); return materialData.basic; }
    const PhongMaterialData& getPhongMaterial() const { _ASSERTE(materialType == Type::PHONG); return materialData.phong; }
    const PBRPhongMaterialData& getPBRPhongMaterial() const { _ASSERTE(materialType == Type::PBR_PHONG); return materialData.pbrPhong; }

    Type getMaterialType() const { return materialType; }

};

