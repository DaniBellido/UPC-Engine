#pragma once

namespace tinygltf { class Model;  struct Mesh; struct Primitive; }

struct Vertex
{
    Vector3 position;
    Vector2 texCoord0;
};

class Mesh
{
public:

    Mesh() = default;

    // Vertex Buffer (primitive vertex attributes POSITION, NORMAL, TEXCOORD_0...)
    ComPtr<ID3D12Resource> vertexBuffer = nullptr;

    // Index Buffer (primitive indices)
    ComPtr<ID3D12Resource> indexBuffer = nullptr;

    // Views
    D3D12_VERTEX_BUFFER_VIEW vertexView{};
    D3D12_INDEX_BUFFER_VIEW indexView{};

    // Counters
    uint32_t numVertices = 0;
    uint32_t numIndices = 0; 

    int materialIndex = -1;

    void load(const tinygltf::Model& model, const tinygltf::Mesh& gltfMesh, const tinygltf::Primitive& primitive);

};

