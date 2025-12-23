#pragma once

#include "Mesh.h"
#include "BasicMaterial.h"

namespace tinygltf { class Model;  class Node; }

class Model
{
private:


public:
	void Load(const char* assetFileName);

	std::vector<Mesh> meshes;
	std::vector<BasicMaterial> materials;

	Matrix modelMatrix = Matrix::Identity;

};

