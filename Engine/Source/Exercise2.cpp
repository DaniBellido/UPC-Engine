#include "Globals.h"
#include "Exercise2.h"
#include "D3D12Module.h"
#include "ResourcesModule.h"
#include "Application.h"
#include <d3d12.h>
#include "ReadData.h"


bool Exercise2::init()
{
	createVertexBuffer();
	createRootSignature();
	createPSO();

	return true;
}

void Exercise2::render()
{
    // D3D12Module handles:
    // - preRender(): resets the command allocator and list, transitions the back buffer to RENDER_TARGET,
    //                and sets the render target (OMSetRenderTargets)
    // - postRender(): transitions the back buffer to PRESENT, closes the command list,
    //                 executes it, and presents the frame.
    //  Only need to:
    // - Set pipeline and root signature
    // - Set buffers and topology
    // - Issue draw calls


    // We get the command list that has already been reset and prepared by preRender() in D3D12Module
    // (preRender() does: Reset allocator, Reset commandList, transition to RENDER_TARGET, OMSetRenderTargets)
    D3D12Module* d3d12 = app->getD3D12();
    ID3D12GraphicsCommandList* commandList = d3d12->getCommandList();

    // ------------------------------------------------------------
    // Set viewport and scissor rect
    // ------------------------------------------------------------
    
    // The viewport defines the area of the screen where pixels are drawn
    D3D12_VIEWPORT viewport{ 0.0f, 0.0f, float(d3d12->getWindowWidth()), float(d3d12->getWindowHeight()), 0.0f, 1.0f };
    D3D12_RECT scissor{ 0, 0, LONG(d3d12->getWindowWidth()), LONG(d3d12->getWindowHeight()) };
    // Assign viewport and scissor to the rasterizer stage
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissor);

    // ------------------------------------------------------------
    // Set up the graphics pipeline
    // ------------------------------------------------------------
  
    // The root signature defines how resources are passed to shaders
    commandList->SetGraphicsRootSignature(rootSignature.Get());
    // The PSO (Pipeline State Object) contains shaders, input layout, blend state, rasterizer state, etc.
    commandList->SetPipelineState(pso.Get());

    // ------------------------------------------------------------
    // Configure the Input Assembler
    // ------------------------------------------------------------
    
    // Set primitive type (triangles)
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    // Assign the vertex buffer containing the triangle’s vertices
    commandList->IASetVertexBuffers(0, 1, &vertexBufferView);

    // ------------------------------------------------------------
    // Draw the geometry
    // ------------------------------------------------------------
    // DrawInstanced(numVertices, numInstances, startVertex, startInstance)
    // Here we draw 3 vertices forming a single triangle
    commandList->DrawInstanced(3, 1, 0, 0);

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
