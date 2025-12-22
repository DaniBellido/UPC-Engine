#include "Globals.h"
#include "Mesh.h"

#include "my_gltf.h"

void Mesh::load(const tinygltf::Model& model, const tinygltf::Mesh& gltfMesh, const tinygltf::Primitive& primitive)
{
	const auto& itPos = primitive.attributes.find("POSITION");
	if (itPos != primitive.attributes.end()) // If no position no geometry data
	{
		uint32_t numVertices = uint32_t(model.accessors[itPos->second].count);
		Vertex* vertices = new Vertex[numVertices];
		uint8_t* vertexData = (uint8_t*)vertices; // Casts Vertex Buffer to Bytes (uint8_t*) buffer
		loadAccessorData(vertexData + offsetof(Vertex, position), sizeof(Vector3), sizeof(Vertex),
			numVertices, model, itPos->second);
		loadAccessorData(vertexData + offsetof(Vertex, texCoord0), sizeof(Vector2), sizeof(Vertex),
			numVertices, model, primitive.attributes, "TEXCOORD_0");
	}
}

bool Mesh::loadAccessorData(uint8_t* data, size_t elemSize, size_t stride, size_t elemCount, const tinygltf::Model& model, int accesorIndex)
{

	// Implementation for loading accessor data and store it into given data buffer

	return false;
}

bool Mesh::loadAccessorData(uint8_t* data, size_t elemSize, size_t stride, size_t elemCount, const tinygltf::Model& model, const std::map<std::string, int>& attributes, const char* accesorName)
{
	const auto& it = attributes.find(accesorName);
	if (it != attributes.end())
	{
		return loadAccessorData(data, elemSize, stride, elemCount, model, it->second);
	}
	return false;
}

