#pragma once

#include "Mesh.h"
#include "BasicMaterial.h"

namespace tinygltf { class Model;  class Node; }

class Model
{
private:
    std::vector<Mesh> meshes;
    std::vector<BasicMaterial> materials;

    Matrix modelMatrix = Matrix::Identity;

public:
    Model();
    ~Model();

	bool Load(const char* folderName, const char* assetFileName);

    const std::vector<Mesh>& getMeshes()   const { return meshes; }
    const std::vector<BasicMaterial>& getMaterials() const { return materials; }

    const Matrix& getModelMatrix() const { return modelMatrix; }
    void          setModelMatrix(const Matrix& m) { modelMatrix = m; }

    const BasicMaterial& getMaterialForMesh(size_t i) const
    {
        const Mesh& m = meshes[i];
        return materials[m.getMaterialIndex()];
    }

    // Accesos directos útiles
    size_t getMeshCount() const { return meshes.size(); }
    const Mesh& getMesh(size_t i) const { return meshes[i]; }

};

