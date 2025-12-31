#include "Globals.h"
#include "BasicMaterial.h"
#include "Application.h"
#include "ResourcesModule.h"
#include "ShaderDescriptorsModule.h"

#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_EXTERNAL_IMAGE 

#include "tiny_gltf.h"

void BasicMaterial::load(const tinygltf::Model& model, const tinygltf::Material& material, const char* basePath)
{
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

	// CBV
	BasicMaterialData data = { baseColour, hasColourTexture, {} };
	materialBuffer = app->getResources()->createDefaultBuffer(&data, alignUp(sizeof(BasicMaterialData), 256), "MaterialCBV");
}
