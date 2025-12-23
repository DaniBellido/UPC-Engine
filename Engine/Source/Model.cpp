#include "Globals.h"
#include "Model.h"

#include "Mesh.h"
#include "BasicMaterial.h"

#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_EXTERNAL_IMAGE 

#include "tiny_gltf.h"

bool Model::Load(const char* assetFileName)
{
    Logger::Warn("Searching: " + std::string(assetFileName));

	tinygltf::TinyGLTF gltfContext;
	tinygltf::Model model;
	std::string error, warning;
	bool loadOk = gltfContext.LoadASCIIFromFile(&model, &error, &warning, assetFileName);

    if (!loadOk) 
    {
        Logger::Err("ERROR tinygltf: " + error);
        Logger::Warn("WARNING: " + warning);
        return false;
    }

    // Load Material
    for (const auto& mat : model.materials) {
        BasicMaterial newMat;
        newMat.load(model, mat, "assets/");
        materials.push_back(newMat);
    }

    // Load Mesh
    for (const auto& gltfMesh : model.meshes) {
        for (const auto& prim : gltfMesh.primitives) {
            Mesh newMesh;
            newMesh.load(model, gltfMesh, prim);
            newMesh.materialIndex = prim.material;
            meshes.push_back(newMesh);
        }
    }

    Logger::Log("FINISHED - Meshes: " + std::to_string(meshes.size()) + ", Materials: " + std::to_string(materials.size()));
    return true;
}
