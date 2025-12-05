#include "Globals.h"
#include "Exercise3.h"

#include "D3D12Module.h"
#include "ResourcesModule.h"
#include "Application.h"

#include <d3d12.h>
#include "ReadData.h"
#include <d3dcompiler.h>
#include "d3dx12.h"


bool Exercise3::init()
{
    createVertexBuffer();
    createRootSignature();
    createPSO();

    return true;
}

void Exercise3::render()
{
    // ----------------------------------------------------------------
    // ImGui Window
    // ----------------------------------------------------------------
    ImGui::Begin("3D Rotation");
    ImGui::SliderFloat("Rot X", &rotationX, 0.0f, 360.0f);
    ImGui::SliderFloat("Rot Y", &rotationY, 0.0f, 360.0f);
    ImGui::SliderFloat("Rot Z", &rotationZ, 0.0f, 360.0f);

    ImGui::SliderFloat("Camera Distance", &camDistance, 1.0f, 20.0f);
    ImGui::End();

    // ------------------------------------------------------------
    D3D12Module* d3d12 = app->getD3D12();
    ID3D12GraphicsCommandList* commandList = d3d12->getCommandList();

    // ------------------------------------------------------------
    // Set viewport and scissor rect
    // ------------------------------------------------------------
    D3D12_VIEWPORT viewport{ 0.0f, 0.0f, float(d3d12->getWindowWidth()), float(d3d12->getWindowHeight()), 0.0f, 1.0f };  // The viewport defines the area of the screen where pixels are drawn
    D3D12_RECT scissor{ 0, 0, LONG(d3d12->getWindowWidth()), LONG(d3d12->getWindowHeight()) };
    commandList->RSSetViewports(1, &viewport);                  // Assign viewport and scissor to the rasterizer stage
    commandList->RSSetScissorRects(1, &scissor);

    // ------------------------------------------------------------
    // Clearing RTV & DSV
    // ------------------------------------------------------------
    float clearColor[] = { 0.2f, 0.2f, 0.2f, 1.0f };
    D3D12_CPU_DESCRIPTOR_HANDLE rtv = d3d12->getRenderTargetDescriptor();
    D3D12_CPU_DESCRIPTOR_HANDLE dsv = d3d12->getDepthStencilDescriptor();

    commandList->OMSetRenderTargets(1, &rtv, false, &dsv);
    commandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
    commandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

    // ------------------------------------------------------------
    // Set up the graphics pipeline
    // ------------------------------------------------------------
    commandList->SetGraphicsRootSignature(rootSignature.Get()); // The root signature defines how resources are passed to shaders
    commandList->SetPipelineState(pso.Get());                   // The PSO (Pipeline State Object) contains shaders, input layout, blend state, rasterizer state, etc.

    // ------------------------------------------------------------
    // Configure the Input Assembler
    // ------------------------------------------------------------
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST); // Set primitive type (triangles)
    commandList->IASetVertexBuffers(0, 1, &vertexBufferView);                 // Assign the vertex buffer containing the triangle’s vertices

    // ----------------------------------------------------------------
    // Model-View-Projection Matrix Pipeline
    // ----------------------------------------------------------------
    SimpleMath::Matrix rotX = SimpleMath::Matrix::CreateRotationX(DegreesToRadians(rotationX));
    SimpleMath::Matrix rotY = SimpleMath::Matrix::CreateRotationY(DegreesToRadians(rotationY));
    SimpleMath::Matrix rotZ = SimpleMath::Matrix::CreateRotationZ(DegreesToRadians(rotationZ));

    SimpleMath::Matrix model = rotX * rotY * rotZ;
    SimpleMath::Matrix view = SimpleMath::Matrix::CreateLookAt(
        SimpleMath::Vector3(0.0f, 3.0f, camDistance),SimpleMath::Vector3::Zero,SimpleMath::Vector3::Up);

    float aspect = float(d3d12->getWindowWidth()) / float(d3d12->getWindowHeight());
    SimpleMath::Matrix proj = SimpleMath::Matrix::CreatePerspectiveFieldOfView(XM_PIDIV4, aspect, 0.1f, 100.0f);

    mvpMatrix = (model * view * proj).Transpose();
    commandList->SetGraphicsRoot32BitConstants(0, 64, &mvpMatrix, 0);


    // ------------------------------------------------------------
    // GRID
    // ------------------------------------------------------------

    dd::xzSquareGrid(-10.0f, 10.0f, 0.0f, 1.0f, dd::colors::LightGray);
    dd::axisTriad(ddConvert(SimpleMath::Matrix::Identity), 0.1f, 1.0f);

    // ------------------------------------------------------------
    // Draw the geometry
    // ------------------------------------------------------------
    commandList->DrawInstanced(3, 1, 0, 0);   // DrawInstanced(numVertices, numInstances, startVertex, startInstance)

    app->getDebugDrawPass()->record(commandList, app->getD3D12()->getWindowWidth(), app->getD3D12()->getWindowHeight(), view, proj);
}

bool Exercise3::createVertexBuffer()
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

    vertexBuffer = app->getResources()->createDefaultBuffer(vertices, sizeof(vertices), "Exercise3");

    vertexBufferView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
    vertexBufferView.StrideInBytes = sizeof(Vertex);
    vertexBufferView.SizeInBytes = sizeof(vertices);

    return true;
}

bool Exercise3::createRootSignature()
{
    // Aquí defines un root parameter de constantes con el tamaño del matrix (16 floats)
    CD3DX12_ROOT_PARAMETER rootParameter;
    //rootParameter.InitAsConstants(sizeof(SimpleMath::Matrix) / sizeof(UINT32), 0); // 0 = register b0
    rootParameter.InitAsConstants(64, 0);  // 64 DWORDs para matriz 4x4

    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Init(1, &rootParameter, 0, nullptr,
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> signatureBlob;
    ComPtr<ID3DBlob> errorBlob;
    HRESULT hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1,
        &signatureBlob, &errorBlob);
    if (FAILED(hr)) {
        // Manejo error
        return false;
    }

    hr = app->getD3D12()->getDevice()->CreateRootSignature(0,
        signatureBlob->GetBufferPointer(),
        signatureBlob->GetBufferSize(),
        IID_PPV_ARGS(&rootSignature));
    return SUCCEEDED(hr);
}

bool Exercise3::createPSO()
{
    D3D12_INPUT_ELEMENT_DESC inputLayout[] = { {"MY_POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0} };

    auto dataVS = DX::ReadData(L"Exercise3VS.cso");
    auto dataPS = DX::ReadData(L"Exercise3PS.cso");

    if (dataVS.empty() || dataPS.empty()) {
        Logger::Err("ERROR: VS or PS .cso is empty — check build output and paths");
        return false;
    }
    else
    {
        Logger::Log("Exercise3: VS Data & PS Data: OK");
    }

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { inputLayout, sizeof(inputLayout) / sizeof(D3D12_INPUT_ELEMENT_DESC) };  // the structure describing our input layout
    psoDesc.pRootSignature = rootSignature.Get();                                                   // the root signature that describes the input data this pso needs
    psoDesc.VS = { dataVS.data(), dataVS.size() };                                                  // structure describing where to find the vertex shader bytecode and how large it is
    psoDesc.PS = { dataPS.data(), dataPS.size() };                                                  // same as VS but for pixel shader
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;                         // type of topology we are drawing
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;                                             // format of the render target
    psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;                    //  AÑADE
    psoDesc.SampleDesc = { 1, 0 };                                                                  // must be the same sample description as the swapchain and depth/stencil buffer
    psoDesc.SampleMask = 0xffffffff;                                                                // sample mask has to do with multi-sampling. 0xffffffff means point sampling is done
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);                               // a default rasterizer state.
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);                                         // a default blend state.
    psoDesc.NumRenderTargets = 1;                                                                   // we are only binding one render target
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);  //  AÑADE

    // create the pso
    return SUCCEEDED(app->getD3D12()->getDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso)));
}


