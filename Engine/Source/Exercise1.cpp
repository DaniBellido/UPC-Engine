#include "Globals.h"
#include "Exercise1.h"
#include "D3D12Module.h"
#include "Application.h"

void Exercise1::render()
{
    D3D12Module* d3d12 = app->getD3D12();
    ID3D12GraphicsCommandList* cmd = d3d12->getCommandList();

    cmd->ClearRenderTargetView(d3d12->getRenderTargetDescriptor(), bgColor, 0, nullptr);

    // Change color from editor
    ImGui::Begin("Background Color");
    ImGui::ColorEdit3("Color", bgColor);

    ImGui::Text("Color Wheel Picker");
    ImGui::ColorPicker3("Color", bgColor);
    ImGui::End();

}
