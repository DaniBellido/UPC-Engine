#include "Globals.h"
#include "Model.h"

#include "Mesh.h"
#include "BasicMaterial.h"

#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_EXTERNAL_IMAGE 
#define TINYGLTF_IMPLEMENTATION /* Only in one of the includes */

#include "tiny_gltf.h"

Model::Model()
{
}

Model::~Model()
{
}

bool Model::Load(const char* folderName, const char* assetFileName)
{
    std::string fullPath = std::string(folderName) + "/" + assetFileName;

    // TEST if file exists
    Logger::Warn("Searching: " + std::string(fullPath));
    std::ifstream testFile(fullPath, std::ios::binary);

    if (!testFile.is_open()) 
    {
        Logger::Err("File does not exist: " + std::string(fullPath));
    }
    else 
    {
        Logger::Log("File found!");
        testFile.close();
    }
    
	tinygltf::TinyGLTF gltfContext;
	tinygltf::Model model;
	std::string error, warning;
    bool loadOk = gltfContext.LoadASCIIFromFile(&model, &error, &warning, fullPath);

    Logger::Log("RESULT: loadOk=" + std::to_string((int)loadOk) + " | error_len=" + std::to_string(error.size()));

    if (!loadOk) 
    {
        Logger::Err("tinygltf: " + error);
        Logger::Warn("tinygltf: " + warning);
        return false;
    }

    // Load Material
    for (const auto& mat : model.materials) {
        BasicMaterial newMat;
        newMat.load(model, mat, folderName);  
        materials.push_back(newMat);
    }

    Logger::Log("=== MATERIALS DEBUG ===");
    for (size_t i = 0; i < materials.size(); i++)
    {
        std::string hasTexStr = materials[i].hasTexture() ? "TRUE" : "FALSE";
        std::string bufferStr = materials[i].getMaterialBuffer() ? "OK" : "NULL";
        Logger::Log("Mat[" + std::to_string(i) + "]: hasTex=" + hasTexStr +
            " srv=" + std::to_string(materials[i].getColourTexSRV()) +
            " buffer=" + bufferStr);
    }
    Logger::Log("=== END MATERIALS DEBUG ===");

    // Load Mesh
    for (const auto& gltfMesh : model.meshes) {
        for (const auto& prim : gltfMesh.primitives) {
            Mesh newMesh;
            newMesh.load(model, gltfMesh, prim);
            newMesh.setMaterialIndex(prim.material);
            meshes.push_back(newMesh);
        }
    }

    Logger::Log("FINISHED - Meshes: " + std::to_string(meshes.size()) + ", Materials: " + std::to_string(materials.size()));
    return true;
}
