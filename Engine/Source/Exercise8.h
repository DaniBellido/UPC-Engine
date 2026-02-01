#pragma once
#include "Module.h"
#include "DebugDrawPass.h"
#include "Model.h"
#include "ImGuizmo.h"

class CameraModule;
class ShaderDescriptorsModule;
class SamplersModule;

class Exercise8 : public Module
{
private:

	// ------------------------------------------------------------------------
	// GPU Constant Buffers
	// ------------------------------------------------------------------------
	struct PerInstance
	{
		SimpleMath::Matrix modelMat;
		SimpleMath::Matrix normalMat;
		PBRPhongMaterialData  material;
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

		// Point light data
		SimpleMath::Vector3 pointPos;
		float pointRange;

		SimpleMath::Vector3 pointColor;
		float pointIntensity;

	};

	// ------------------------------------------------------------------------
	// Pipeline
	// ------------------------------------------------------------------------
	ComPtr<ID3D12RootSignature> rootSignature;
	ComPtr<ID3D12PipelineState> pso;
	ComPtr<ID3D12PipelineState> psoWireframe;
	ComPtr<ID3D12PipelineState> psoNormals;

	// ------------------------------------------------------------------------
	// Scene
	// -----------------------------------------------------------------------
	SimpleMath::Matrix mvpMatrix;
	std::unique_ptr<Model> duck;

	SimpleMath::Quaternion qRot = SimpleMath::Quaternion::Identity;
	float rotationX{ 90.0f }, rotationY{ 0.0f }, rotationZ{ 0.0f };
	float scaleX{ 1.0f }, scaleY{ 1.0f }, scaleZ{ 1.0f };
	float positionX{ 0.0f }, positionY{ 0.0f }, positionZ{ 0.0f };

	// ------------------------------------------------------------------------
	// Directional Light (PerFrame)
	// ------------------------------------------------------------------------
	SimpleMath::Vector3 lightDir = SimpleMath::Vector3(0.5f, -1.0f, 0.5f);
	SimpleMath::Vector3 lightColor = SimpleMath::Vector3(1.0f, 1.0f, 1.0f);
	SimpleMath::Vector3 ambient = SimpleMath::Vector3(0.25f, 0.25f, 0.25f);

	// ------------------------------------------------------------------------
	// Point Light (PerFrame)
	// ------------------------------------------------------------------------
	SimpleMath::Vector3 pointPosition = { 0, 3, 0 };
	float pointRange = 5.0f;
	SimpleMath::Vector3 pointColor = SimpleMath::Vector3(0.0f, 0.0f, 1.0f);
	float pointIntensity = 10.0f;

	// ------------------------------------------------------------------------
	// PBR Phong material overrides (PerInstance)
	// ------------------------------------------------------------------------
	float pbrPhongShininess = 64.0f;
	SimpleMath::Vector4 pbrPhongDiffuseColor = { 1.0f, 1.0f, 1.0f, 1.0f };
	SimpleMath::Vector4 pbrPhongSpecularColor = { 0.015f, 0.015f, 0.015f, 0.0f };

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
	bool isPointLightGizmoVisible = true;
	bool isWireframe = false;
	bool isWireframeOverlay = false;
	bool isNormalsVisible = false;

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
	Exercise8();
	~Exercise8();

	bool init() override;
	void render() override;

};

