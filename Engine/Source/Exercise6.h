#pragma once
#include "Module.h"
#include "DebugDrawPass.h"
#include "Model.h"
#include "ImGuizmo.h"

class CameraModule;
class ShaderDescriptorsModule;
class SamplersModule;


class Exercise6 : public Module
{
private:

	// ------------------------------------------------------------------------
	// GPU Constant Buffers
	// ------------------------------------------------------------------------
	struct PerInstance
	{
		SimpleMath::Matrix modelMat;
		SimpleMath::Matrix normalMat;
		PhongMaterialData  material;
	};

	struct PerFrame
	{
		SimpleMath::Vector3 L;
		float pad0;
		SimpleMath::Vector3 Lc;
		float pad1;
		SimpleMath::Vector3 Ac;
		float pad2;
		SimpleMath::Vector3 viewPos;
		float pad3;
	};

	// ------------------------------------------------------------------------
	// Pipeline
	// ------------------------------------------------------------------------
	ComPtr<ID3D12RootSignature> rootSignature;
	ComPtr<ID3D12PipelineState> pso;

	// ------------------------------------------------------------------------
	// Scene
	// -----------------------------------------------------------------------
	SimpleMath::Matrix mvpMatrix;
	std::unique_ptr<Model> duck;

	SimpleMath::Quaternion qRot = SimpleMath::Quaternion::Identity;
	float rotationX{ 0.0f }, rotationY{ 0.0f }, rotationZ{ 0.0f };
	float scaleX{ 0.01f }, scaleY{ 0.01f }, scaleZ{ 0.01f };
	float positionX{ 0.0f }, positionY{ 0.0f }, positionZ{ 0.0f };

	// ------------------------------------------------------------------------
	// Lighting (PerFrame)
	// ------------------------------------------------------------------------
	SimpleMath::Vector3 lightDir = SimpleMath::Vector3(0.5f, -1.0f, 0.5f);
	SimpleMath::Vector3 lightColor = SimpleMath::Vector3(1.0f, 1.0f, 1.0f);
	SimpleMath::Vector3 ambient = SimpleMath::Vector3(0.25f, 0.25f, 0.25f);

	// ------------------------------------------------------------------------
	// Phong material overrides (PerInstance)
	// ------------------------------------------------------------------------
	float phongKd = 1.0f;
	float phongKs = 0.4f;
	float phongShininess = 32.0f;
	SimpleMath::Vector4 phongDiffuseColor = { 1.0f, 1.0f, 1.0f, 1.0f };

	enum class MaterialPreset
	{
		Custom = 0,
		Matte,
		Plastic,
		Metal,
		Rubber
	};

	MaterialPreset currentPreset = MaterialPreset::Custom;

	// ------------------------------------------------------------------------
	// Editor / Debug flags
	// ------------------------------------------------------------------------
	bool isGridVisible = true;
	bool isAxisVisible = true;
	bool isGeoVisible = true;
	bool isGizmoVisible = true;
	bool isTextureVisible = true;
	bool isLightGizmoVisible = true;

	ImGuizmo::OPERATION currentOperation = ImGuizmo::TRANSLATE;

	// Editor camera overrides
	float camSpeed = 5.0f;
	float camFov = XM_PIDIV4;
	float camNear = 0.1f;
	float camFar = 100.0f;

	// ------------------------------------------------------------------------
	// Internal helpers
	// ------------------------------------------------------------------------
	bool createRootSignature();
	bool createPSO();
	void drawModel(ID3D12GraphicsCommandList* commandList, ShaderDescriptorsModule* shaders, SamplersModule* samplers);
	void ApplyImGuizmo(CameraModule* camera);
	void applyMaterialPreset(MaterialPreset preset);
	void ExerciseMenu(CameraModule* camera);
	

public:
	Exercise6();
	~Exercise6();

	bool init() override;
	void render() override;
};

