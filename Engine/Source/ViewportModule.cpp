#include "Globals.h"
#include "ViewportModule.h"

ViewportModule::ViewportModule(HWND hWnd, D3D12Module* d3d12) 
{
    this->hWnd = hWnd;
    this->d3d12 = d3d12;
}

bool ViewportModule::init()
{
    Logger::Log("Initializing Viewport...");
	return true;
}

void ViewportModule::preRender()
{
    if (!visible)
        return;

    // Aquí se define la ventana del Viewport
    ImGui::Begin("Viewport", &visible);

    // Más adelante aquí dibujaremos la textura del render 3D
    ImGui::Text("Render the 3D world here");

    //ID3D12DescriptorHeap* heaps[] = { d3d12->getRtvDescriptorHeap()};
    //d3d12->getCommandList()->SetDescriptorHeaps(_countof(heaps), heaps);

    //D3D12_GPU_DESCRIPTOR_HANDLE srvHandle = d3d12->getSceneSRVGpuHandle();
    //ImGui::Image((ImTextureID)srvHandle.ptr, size);

    ImVec2 size = ImGui::GetContentRegionAvail();
    ImGui::Text("Viewport size: %.0fx%.0f", size.x, size.y);
    ImGui::Text("Render target: %dx%d", d3d12->getWindowWidth(), d3d12->getWindowHeight());

    ImGui::End();
}

void ViewportModule::render()
{
}

bool ViewportModule::cleanUp()
{
	return true;
}
