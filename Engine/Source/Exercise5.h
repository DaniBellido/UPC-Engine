#pragma once
#include "Module.h"
#include "DebugDrawPass.h"
#include "Model.h"

class CameraModule;
class ShaderDescriptorsModule;
class SamplersModule;

class Exercise5 : public Module
{
private:

	ComPtr<ID3D12RootSignature> rootSignature;
	ComPtr<ID3D12PipelineState> pso;
	ComPtr<ID3D12Resource> vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
	SimpleMath::Matrix mvpMatrix;

	ComPtr<ID3D12Resource> indexBuffer;
	D3D12_INDEX_BUFFER_VIEW indexBufferView;

	ComPtr<ID3D12Resource> texture;
	UINT textureIndex = UINT_MAX;

	uint32_t samplerIndices[4] = { 0,1,2,3 };
	int samplerMode = 0;

	std::unique_ptr<Model> duck;
	float rotationX{ 0.0f }, rotationY{ 0.0f }, rotationZ{ 0.0f };
	float scaleX{ 0.01f }, scaleY{ 0.01f }, scaleZ{ 0.01f };
	float positionX{ 0.0f }, positionY{ 0.0f }, positionZ{ 0.0f };

	float camSpeed = 5.0f;
	float camHeight = 3.0f;
	float camSide = 0.0f;

	float camFov = XM_PIDIV4;
	float camNear = 0.1f;
	float camFar = 100.0f;

	bool isGridVisible = true;
	bool isAxisVisible = true;
	bool isGeoVisible = true;

	bool createRootSignature();
	bool createPSO();

	void loadModel(ID3D12GraphicsCommandList* commandList, ShaderDescriptorsModule* shaders, SamplersModule* samplers);

	void ExerciseMenu(CameraModule* camera);

	float DegreesToRadians(float degrees) { return degrees * 3.14159265359f / 180.0f; }


public:
	Exercise5();
	~Exercise5();

	bool init() override;
	void render() override;
};

