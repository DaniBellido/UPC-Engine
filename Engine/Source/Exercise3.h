#pragma once
#include "Module.h"
class Exercise3 : public Module
{
private:
	ComPtr<ID3D12Resource>      vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW    vertexBufferView;
	ComPtr<ID3D12Resource>      bufferUploadHeap;
	ComPtr<ID3D12RootSignature> rootSignature;
	ComPtr<ID3D12PipelineState> pso;

	bool createVertexBuffer();
	bool createRootSignature();
	bool createPSO();

public:

	bool init() override;
	void render() override;
};

