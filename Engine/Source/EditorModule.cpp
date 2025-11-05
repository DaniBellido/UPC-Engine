#include "Globals.h"
#include "EditorModule.h"
#include "D3D12Module.h"


EditorModule::EditorModule(HWND hWnd, D3D12Module* d3d12)
{
	this->hWnd = hWnd;
	this->d3d12 = d3d12;
}

bool EditorModule::init() 
{
	imGuiPass = new ImGuiPass(d3d12->getDevice(), hWnd, cpuHandle, gpuHandle);

	return true;
}

void EditorModule::preRender()
{
	imGuiPass->startFrame();
	ImGui::ShowDemoWindow();
}

void EditorModule::render()
{
	imGuiPass->record(d3d12->getCommandList(), d3d12->getRenderTargetDescriptor());
}

void EditorModule::postRender()
{

}

bool EditorModule::cleanUp()
{
	delete imGuiPass;
	imGuiPass = nullptr;
	return true;
}