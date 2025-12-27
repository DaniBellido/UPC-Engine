#pragma once

namespace tinygltf { class Model;  struct Mesh; struct Primitive; }

struct Vertex
{
    Vector3 position;
    Vector2 texCoord0;
};

class Mesh
{
private:
    ComPtr<ID3D12Resource> vertexBuffer;
    ComPtr<ID3D12Resource> indexBuffer;

    D3D12_VERTEX_BUFFER_VIEW vertexView{};
    D3D12_INDEX_BUFFER_VIEW indexView{};

    uint32_t numVertices = 0;
    uint32_t numIndices = 0;

    int materialIndex = -1;

public:

    Mesh() = default;

    const D3D12_VERTEX_BUFFER_VIEW& getVertexView() const { return vertexView; }
    const D3D12_INDEX_BUFFER_VIEW& getIndexView()  const { return indexView; }
    uint32_t getVertexCount() const { return numVertices; }
    uint32_t getIndexCount()  const { return numIndices; }
    int      getMaterialIndex() const { return materialIndex; }

    bool hasIndices() const { return numIndices > 0; }

    void setMaterialIndex(int idx) { materialIndex = idx; }

    void load(const tinygltf::Model& model, const tinygltf::Mesh& gltfMesh, const tinygltf::Primitive& primitive);


};

