#pragma once
#include "Globals.h"
#include "Module.h"
#include "ImGuiPass.h"
#include "D3D12Module.h"
#include "ConsoleModule.h"

class EditorModule : public Module
{

private:
	HWND hWnd = NULL;

	D3D12Module* d3d12 = nullptr;
	ImGuiPass* imGuiPass = nullptr;
	ConsoleModule* console = nullptr;

	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = {};
	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = {};

public:

	EditorModule(HWND hWnd, D3D12Module* d3d12);
	~EditorModule() {}

	bool init() override;
	//void update() override;
	void preRender() override;
	void postRender() override;
	void render() override;
	bool cleanUp() override;

private:
	// TODO: private methods
};

