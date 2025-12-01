#pragma once
#include "Module.h"
#include "DebugDrawPass.h"

class Exercise3 : public Module
{
private:
	/*ComPtr<ID3D12Resource>      vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW    vertexBufferView;
	ComPtr<ID3D12Resource>      bufferUploadHeap;*/

	ComPtr<ID3D12RootSignature> rootSignature;
	ComPtr<ID3D12PipelineState> pso;
	ComPtr<ID3D12Resource> vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
	SimpleMath::Matrix mvpMatrix;

	float rotationX = 0.0f;
	float rotationY = 0.0f;
	float rotationZ = 0.0f;
	float camDistance = 5.0f;

	DebugDrawPass* debugDrawPass = nullptr;

	bool createVertexBuffer();
	bool createRootSignature();
	bool createPSO();

	float DegreesToRadians(float degrees)
	{
		return degrees * 3.14159265359f / 180.0f;
	}

public:

	bool init() override;
	void render() override;
};

