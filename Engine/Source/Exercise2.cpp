#include "Globals.h"
#include "Exercise2.h"
#include "D3D12Module.h"
#include "ResourcesModule.h"
#include "Application.h"
#include <d3d12.h>
#include "ReadData.h"
#include "SceneRenderPass.h"

bool Exercise2::init()
{
	createVertexBuffer();
	createRootSignature();
	createPSO();

	return true;
}

void Exercise2::render()
{
    // ------------------------------------------------------------
    // Grab the D3D12 module + command list for this frame
    // (the command list is already open / recording at this point)
    // ------------------------------------------------------------
    D3D12Module* d3d12 = app->getD3D12();
    ID3D12GraphicsCommandList* commandList = d3d12->getCommandList();

    // ------------------------------------------------------------
    // SceneRenderPass decides WHERE we render this frame:
    //   - If the viewport is active/usable -> render into the viewport texture
    //   - Otherwise                         -> render into the swapchain backbuffer
    //
    // It also carries the correct width/height/aspect for the chosen target
    // ------------------------------------------------------------
    SceneRenderPass pass = GetSceneRenderPass(app);

    // ------------------------------------------------------------
    // Begin the render pass:
    //   - Transitions the selected color target into RENDER_TARGET state
    //   - Binds RTV (and DSV if available) to the OM stage
    //   - Sets viewport + scissor to match the target dimensions
    //   - Optionally clears the RTV (and clears DSV if the pass does that)
    //
    // IMPORTANT (Debug Layer / Performance):
    // The viewport color texture was created with an optimized clear value,
    // clearing with a different color triggers warning ID3D12CommandList::ClearRenderTargetView due to a missmatch
    // The viewport module in this project creates the texture with (0.2,0.2,0.2,1),
    // so we clear with that exact same value to keep it "optimized" and warning-free
    // ------------------------------------------------------------
    const float clearColor[] = { 0.2f, 0.2f, 0.2f, 1.0f };
    pass.begin(commandList, clearColor);

    // ------------------------------------------------------------
    // Bind pipeline state:
    //   - Root signature defines what resources/constants are expected
    //   - PSO defines shaders, input layout, render target formats, etc.
    // ------------------------------------------------------------
    commandList->SetGraphicsRootSignature(rootSignature.Get());
    commandList->SetPipelineState(pso.Get());

    // ------------------------------------------------------------
    // Bind geometry:
    // We render a triangle (triangle list topology + vertex buffer)
    // ------------------------------------------------------------
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->IASetVertexBuffers(0, 1, &vertexBufferView);

    // ------------------------------------------------------------
    // Draw the geometry
    // DrawInstanced(vertexCountPerInstance, instanceCount, startVertex, startInstance)
    // Here: 3 vertices -> one triangle
    // ------------------------------------------------------------
    commandList->DrawInstanced(3, 1, 0, 0);

    // ------------------------------------------------------------
    // End the render pass:
    //   - Transitions the color target to the correct state for its next use:
    //       * viewport texture -> typically PIXEL_SHADER_RESOURCE (so ImGui can sample it)
    //       * backbuffer       -> typically PRESENT
    //
    // This is REQUIRED when rendering into the viewport texture; otherwise,
    // the texture may stay in RENDER_TARGET state and break ImGui sampling
    // ------------------------------------------------------------
    pass.end(commandList);

}

bool Exercise2::createVertexBuffer()
{
    struct Vertex
    {
        float x, y, z;
    };

    static Vertex vertices[3] =
    {
        {-1.0f, -1.0f, 0.0f },  // 0
        { 0.0f, 1.0f, 0.0f  },  // 1
        { 1.0f, -1.0f, 0.0f }   // 2
    };

    vertexBuffer = app->getResources()->createDefaultBuffer(vertices, sizeof(vertices), "Exercise2");

    vertexBufferView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
    vertexBufferView.StrideInBytes = sizeof(Vertex);
    vertexBufferView.SizeInBytes = sizeof(vertices);

	return true;
}

bool Exercise2::createRootSignature()
{
    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
    rootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> rootSignatureBlob;

    if (FAILED(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &rootSignatureBlob, nullptr)))
    {
        return false;
    }

    if (FAILED(app->getD3D12()->getDevice()->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(), rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature))))
    {
        return false;
    }

	return true;
}

bool Exercise2::createPSO()
{
    D3D12_INPUT_ELEMENT_DESC inputLayout[] = { {"MY_POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0} };

    auto dataVS = DX::ReadData(L"Exercise2VS.cso");
    auto dataPS = DX::ReadData(L"Exercise2PS.cso");

    if (dataVS.empty() || dataPS.empty()) {
        Logger::Err("ERROR: VS or PS .cso is empty — check build output and paths");
        return false;
    }
    else 
    {
        Logger::Log("Exercise2: VS Data & PS Data: OK");
    }

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { inputLayout, sizeof(inputLayout) / sizeof(D3D12_INPUT_ELEMENT_DESC) };  // the structure describing our input layout
    psoDesc.pRootSignature = rootSignature.Get();                                                   // the root signature that describes the input data this pso needs
    psoDesc.VS = { dataVS.data(), dataVS.size() };                                                  // structure describing where to find the vertex shader bytecode and how large it is
    psoDesc.PS = { dataPS.data(), dataPS.size() };                                                  // same as VS but for pixel shader
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;                         // type of topology we are drawing
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;                                             // format of the render target
    psoDesc.SampleDesc = { 1, 0 };                                                                  // must be the same sample description as the swapchain and depth/stencil buffer
    psoDesc.SampleMask = 0xffffffff;                                                                // sample mask has to do with multi-sampling. 0xffffffff means point sampling is done
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);                               // a default rasterizer state.
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);                                         // a default blend state.
    psoDesc.NumRenderTargets = 1;                                                                   // we are only binding one render target

    // create the pso
    return SUCCEEDED(app->getD3D12()->getDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso)));
}
