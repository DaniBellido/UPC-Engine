#include "Globals.h"
#include "Mesh.h"
#include "Application.h"
#include "ResourcesModule.h"

#include "my_gltf.h"

void Mesh::load(const tinygltf::Model& model, const tinygltf::Mesh& gltfMesh, const tinygltf::Primitive& primitive)
{
	// Find the position attribute within the primitive.
	const auto& itPos = primitive.attributes.find("POSITION");

	if (itPos != primitive.attributes.end()) // If position exists, then there is geometry data
	{
		// Store the number of vertices for later use (DrawInstanced)
		numVertices = uint32_t(model.accessors[itPos->second].count);

		// Get the number of vertices from the accessor corresponding to the position
		uint32_t numVertices = uint32_t(model.accessors[itPos->second].count);

		// Create an array of vertices with the obtained number of vertices
		Vertex* vertices = new Vertex[numVertices];

		// Cast the vertex pointer to a byte pointer for data manipulation
		uint8_t* vertexData = (uint8_t*)vertices; 

		// Load the position accessor data into the vertex's position field
		loadAccessorData(vertexData + offsetof(Vertex, position), sizeof(Vector3), sizeof(Vertex), numVertices, model, itPos->second);

		// Load the texture coordinate data if it exists
		loadAccessorData(vertexData + offsetof(Vertex, texCoord0), sizeof(Vector2), sizeof(Vertex), numVertices, model, primitive.attributes, "TEXCOORD_0");

		// Upload vertex data to GPU using the engine's default buffer creation (DEFAULT heap + staging)
		vertexBuffer = app->getResources()->createDefaultBuffer(vertices, numVertices * sizeof(Vertex), "VertexBuffer");

		// Fill the D3D12_VERTEX_BUFFER_VIEW structure for IASetVertexBuffers
		vertexView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
		vertexView.StrideInBytes = sizeof(Vertex);
		vertexView.SizeInBytes = numVertices * sizeof(Vertex);

		// Store material index for later binding (texture/CBV)
		materialIndex = primitive.material;

		if (primitive.indices >= 0) {
			const tinygltf::Accessor& indAcc = model.accessors[primitive.indices];

			if (indAcc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT ||
				indAcc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT ||
				indAcc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {

				uint32_t indexElementSize = tinygltf::GetComponentSizeInBytes(indAcc.componentType);
				numIndices = uint32_t(indAcc.count);

				// CHECKS DE SEGURIDAD
				if (numIndices > 0 && indexElementSize > 0 && indexElementSize <= 4)
				{
					uint8_t* indices = new uint8_t[numIndices * indexElementSize];

					// Verificar que loadAccessorData funciona
					if (loadAccessorData(indices, indexElementSize, indexElementSize, numIndices, model, primitive.indices)) {
						size_t totalSize = numIndices * indexElementSize;

						auto result = app->getResources()->createDefaultBuffer(indices, totalSize, "IndexBuffer");
						indexBuffer = result;

						if (indexBuffer != nullptr) {
							indexView.BufferLocation = indexBuffer->GetGPUVirtualAddress();
							static const DXGI_FORMAT formats[3] = { DXGI_FORMAT_R8_UINT, DXGI_FORMAT_R16_UINT, DXGI_FORMAT_R32_UINT };
							indexView.Format = formats[(indexElementSize >> 1)];
							indexView.SizeInBytes = UINT(totalSize);
						}
					}
					delete[] indices;
				}
			}
		}
        

		// Free temporary CPU memory after successful GPU upload
		delete[] vertices;
	}

}


