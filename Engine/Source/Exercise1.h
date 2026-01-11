#pragma once
#include "Module.h"
class Exercise1 : public Module
{
private:

    float bgColor[4] = { 1.f, 0.f, 0.f, 1.f };

    ComPtr<ID3D12Resource>      vertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW    vertexBufferView{};

    // Pipeline
    ComPtr<ID3D12RootSignature> rootSignature;
    ComPtr<ID3D12PipelineState> pso;

    bool createVertexBuffer();
    bool createRootSignature();
    bool createPSO();

public:

    bool init() override;
	void render() override;
};

