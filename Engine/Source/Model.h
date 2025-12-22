#pragma once

class Mesh;
class BasicMaterial;

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

