#include "Globals.h"
#include "Exercise1.h"
#include "D3D12Module.h"
#include "Application.h"

void Exercise1::render()
{
    //D3D12Module* d3d12 = app->getD3D12();
    //ID3D12GraphicsCommandList* cmd = d3d12->getCommandList();

    //cmd->ClearRenderTargetView(d3d12->getRenderTargetDescriptor(), bgColor, 0, nullptr);

    //// Change color from editor
    //ImGui::Begin("Background Color");
    //ImGui::ColorEdit3("Color", bgColor);

    //ImGui::Text("Color Wheel Picker");
    //ImGui::ColorPicker3("Color", bgColor);
    //ImGui::End();

    /////////////////////////////////////////
    D3D12Module* d3d12 = app->getD3D12();
    ID3D12GraphicsCommandList* commandList = d3d12->getCommandList();

    commandList->Reset(d3d12->getCommandAllocator(), nullptr);

    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(d3d12->getBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    commandList->ResourceBarrier(1, &barrier);

    float clearColor[] = { 1.0f, 0.0f, 0.0f, 1.0f };
    commandList->ClearRenderTargetView(d3d12->getRenderTargetDescriptor(), clearColor, 0, nullptr);

    barrier = CD3DX12_RESOURCE_BARRIER::Transition(d3d12->getBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    commandList->ResourceBarrier(1, &barrier);

    commandList->Close();
    ID3D12CommandList* commandLists[] = { commandList };
    d3d12->getCommandQueue()->ExecuteCommandLists(UINT(std::size(commandLists)), commandLists);

}
