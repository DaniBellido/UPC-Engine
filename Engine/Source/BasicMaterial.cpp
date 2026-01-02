#include "Globals.h"
#include "BasicMaterial.h"
#include "Application.h"
#include "ResourcesModule.h"
#include "ShaderDescriptorsModule.h"
#include "RingBufferModule.h"

#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_EXTERNAL_IMAGE 

#include "tiny_gltf.h"

void BasicMaterial::load(const tinygltf::Model& model, const tinygltf::Material& material, Type type, const char* basePath)
{
	materialType = type;
	const auto& pbr = material.pbrMetallicRoughness;

	baseColour = Vector4(float(pbr.baseColorFactor[0]), float(pbr.baseColorFactor[1]),
		float(pbr.baseColorFactor[2]), float(pbr.baseColorFactor[3]));

	hasColourTexture = FALSE;
	colourTexSRV = app->getShaderDescriptors()->createNullTexture2DSRV();

	if (pbr.baseColorTexture.index >= 0)
	{
		const tinygltf::Texture& texture = model.textures[pbr.baseColorTexture.index];
		const tinygltf::Image& image = model.images[texture.source];

		if (!image.uri.empty())
		{
			tex = app->getResources()->createTextureFromFile(std::string(basePath) + image.uri);
			colourTexSRV = app->getShaderDescriptors()->createSRV(tex.Get());
			hasColourTexture = TRUE;
		}
	}

    switch (materialType)
    {
        case Type::BASIC:
        {
            materialData.basic = { baseColour, hasColourTexture, {0,0,0} };
            materialBuffer = app->getResources()->createDefaultBuffer(&materialData.basic, sizeof(BasicMaterialData),"MaterialCBV");
            materialBufferGPU = getMaterialBuffer()->GetGPUVirtualAddress();
            break;
    }
        case Type::PHONG:
        {
            materialData.phong.diffuseColour = baseColour;
            materialData.phong.Kd = 1.0f;
            materialData.phong.Ks = 0.5f;
            materialData.phong.shininess = 32.0f;
            materialData.phong.hasDiffuseTex = hasColourTexture;
            break;
        }
        case Type::PBR_PHONG:
        {
            materialData.pbrPhong.diffuseColour = XMFLOAT3(baseColour.x, baseColour.y, baseColour.z);
            materialData.pbrPhong.specularColour = XMFLOAT3(0.5f, 0.5f, 0.5f);
            materialData.pbrPhong.shininess = 32.0f;
            materialData.pbrPhong.hasDiffuseTex = hasColourTexture;
            break;
        }

    }
}
