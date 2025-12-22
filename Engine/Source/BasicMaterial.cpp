#include "Globals.h"
#include "BasicMaterial.h"
#include "Application.h"
#include "ResourcesModule.h"

#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_EXTERNAL_IMAGE 

#include "tiny_gltf.h"

void BasicMaterial::load(const tinygltf::Model& model, const tinygltf::Material& material, const char* basePath)
{
	/*baseColour = Vector4(float(material.baseColorFactor[0]), float(material.baseColorFactor[1]),
		float(material.baseColorFactor[2]), float(material.baseColorFactor[3]));
	if (material.baseColorTexture.index >= 0)
	{
		const tinygltf::Texture& texture = model.textures[material.baseColorTexture.index];
		const tinygltf::Image& image = model.images[texture.source];

		if (!image.uri.empty())
		{
			colourTexSRV = app->getResources()->createTextureFromFile(std::string(basePath) + image.uri);
		}
	}*/
}
