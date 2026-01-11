#include "Globals.h"
#include "Exercise1.h"

#include <d3d12.h>
#include "ReadData.h"
#include "d3dx12.h"

#include "D3D12Module.h"
#include "Application.h"
#include "ResourcesModule.h"
#include "SceneRenderPass.h"



bool Exercise1::init()
{
    createVertexBuffer();
    createRootSignature();
    createPSO();
    return true;
}

void Exercise1::render()
{
    // ------------------------------------------------------------
    // ImGui: dynamic quad color (shader color)
    // ------------------------------------------------------------
    ImGui::Begin("Color Wheel Picker");
    ImGui::ColorEdit3("Color", bgColor);
    ImGui::ColorPicker3("Picker", bgColor);
    ImGui::End();

    bgColor[3] = 1.0f;

    // ------------------------------------------------------------
    // Grab command list
    // ------------------------------------------------------------
    D3D12Module* d3d12 = app->getD3D12();
    ID3D12GraphicsCommandList* commandList = d3d12->getCommandList();

    // ------------------------------------------------------------
    // SceneRenderPass (Viewport texture if usable, else backbuffer)
    // ------------------------------------------------------------
    SceneRenderPass pass = GetSceneRenderPass(app);

    // ------------------------------------------------------------
    // Begin pass: clear first (optimized), then draw quad on top
    // ------------------------------------------------------------
    const float clearColor[] = { 0.2f, 0.2f, 0.2f, 1.0f };
    pass.begin(commandList, clearColor);

    // ------------------------------------------------------------
    // Pipeline
    // ------------------------------------------------------------
    commandList->SetGraphicsRootSignature(rootSignature.Get());
    commandList->SetPipelineState(pso.Get());

    // ------------------------------------------------------------
    // Push shader color (b0) as root constants (float4)
    // ------------------------------------------------------------
    commandList->SetGraphicsRoot32BitConstants(0, 4, bgColor, 0);

    // ------------------------------------------------------------
    // Fullscreen quad
    // ------------------------------------------------------------
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
    commandList->DrawInstanced(4, 1, 0, 0);

    // ------------------------------------------------------------
    // End pass: transitions viewport RTV -> SRV for ImGui sampling
    // ------------------------------------------------------------
    pass.end(commandList);

}

bool Exercise1::createVertexBuffer()
{
    struct Vertex
    {
        float x, y, z;
    };

    // Fullscreen quad in clip space (covers the whole render target)
    static Vertex vertices[4] =
    {
        { -1.0f, -1.0f, 0.0f },
        { -1.0f,  1.0f, 0.0f },
        {  1.0f, -1.0f, 0.0f },
        {  1.0f,  1.0f, 0.0f },
    };

    vertexBuffer = app->getResources()->createDefaultBuffer(vertices, sizeof(vertices), "Exercise1");

    vertexBufferView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
    vertexBufferView.StrideInBytes = sizeof(Vertex);
    vertexBufferView.SizeInBytes = sizeof(vertices);

    return true;
}

bool Exercise1::createRootSignature()
{
    CD3DX12_ROOT_PARAMETER params[1];
    params[0].InitAsConstants(
        4,                              // 4 DWORDs = float4
        0,                              // b0
        0,                              // space0
        D3D12_SHADER_VISIBILITY_PIXEL
    );

    CD3DX12_ROOT_SIGNATURE_DESC desc;
    desc.Init(
        _countof(params),
        params,
        0,
        nullptr,
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
    );

    ComPtr<ID3DBlob> sigBlob;
    ComPtr<ID3DBlob> errBlob;

    HRESULT hr = D3D12SerializeRootSignature(
        &desc, D3D_ROOT_SIGNATURE_VERSION_1, &sigBlob, &errBlob
    );

    if (FAILED(hr))
    {
        if (errBlob)
            Logger::Err((char*)errBlob->GetBufferPointer());
        return false;
    }

    hr = app->getD3D12()->getDevice()->CreateRootSignature(
        0,
        sigBlob->GetBufferPointer(),
        sigBlob->GetBufferSize(),
        IID_PPV_ARGS(&rootSignature)
    );

    return SUCCEEDED(hr);
}

bool Exercise1::createPSO()
{
    D3D12_INPUT_ELEMENT_DESC inputLayout[] =
    {
        { "MY_POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
          D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    auto dataVS = DX::ReadData(L"Exercise1VS.cso");
    auto dataPS = DX::ReadData(L"Exercise1PS.cso");

    if (dataVS.empty() || dataPS.empty())
    {
        Logger::Err("Exercise1: VS or PS .cso is empty — check build output and paths");
        return false;
    }

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { inputLayout, _countof(inputLayout) };
    psoDesc.pRootSignature = rootSignature.Get();
    psoDesc.VS = { dataVS.data(), dataVS.size() };
    psoDesc.PS = { dataPS.data(), dataPS.size() };

    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

    // SceneRenderPass binds a DSV; keep formats consistent
    psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;

    psoDesc.SampleDesc = { 1, 0 };
    psoDesc.SampleMask = 0xffffffff;
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

    // Fullscreen quad background: depth OFF (clean + avoids useless work)
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState.DepthEnable = FALSE;
    psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;

    return SUCCEEDED(app->getD3D12()->getDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso)));
}


