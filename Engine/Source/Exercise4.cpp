#include "Globals.h"
#include "Exercise4.h"

#include "D3D12Module.h"
#include "ResourcesModule.h"
#include "Application.h"

#include <d3d12.h>
#include "ReadData.h"
#include <d3dcompiler.h>
#include "d3dx12.h"

bool Exercise4::init()
{
    createVertexBuffer();
    createIndexBuffer();
    createRootSignature();
    createPSO();

    return true;
}

void Exercise4::render()
{
    // ----------------------------------------------------------------
    // ImGui Window
    // ----------------------------------------------------------------
    ExerciseMenu();

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

    commandList->IASetIndexBuffer(&indexBufferView);

    // ----------------------------------------------------------------
    // Model-View-Projection Matrix Pipeline
    // ----------------------------------------------------------------
    SimpleMath::Matrix rotX = SimpleMath::Matrix::CreateRotationX(DegreesToRadians(rotationX));
    SimpleMath::Matrix rotY = SimpleMath::Matrix::CreateRotationY(DegreesToRadians(rotationY));
    SimpleMath::Matrix rotZ = SimpleMath::Matrix::CreateRotationZ(DegreesToRadians(rotationZ));

    SimpleMath::Matrix scale = SimpleMath::Matrix::CreateScale(scaleX, scaleY, scaleZ);
    SimpleMath::Matrix position = SimpleMath::Matrix::CreateTranslation(positionX, positionY, positionZ);

    SimpleMath::Matrix model = scale * rotX * rotY * rotZ * position;
    SimpleMath::Matrix view = SimpleMath::Matrix::CreateLookAt(
        SimpleMath::Vector3(camSide, camHeight, camDistance), SimpleMath::Vector3::Zero, SimpleMath::Vector3::Up);

    float aspect = float(d3d12->getWindowWidth()) / float(d3d12->getWindowHeight());
    SimpleMath::Matrix proj = SimpleMath::Matrix::CreatePerspectiveFieldOfView(camFov, aspect, camNear, camFar);

    mvpMatrix = (model * view * proj).Transpose();
    commandList->SetGraphicsRoot32BitConstants(0, 64, &mvpMatrix, 0);


    // ------------------------------------------------------------
    // GRID & AXIS
    // ------------------------------------------------------------

    if (isGridVisible) {dd::xzSquareGrid(-10.0f, 10.0f, 0.0f, 1.0f, dd::colors::LightGray);}
    if (isAxisVisible) { dd::axisTriad(ddConvert(SimpleMath::Matrix::Identity), 0.1f, 1.0f); }

    // ------------------------------------------------------------
    // Draw the geometry
    // ------------------------------------------------------------

    if (isGeoVisible) { commandList->DrawIndexedInstanced(36, 1, 0, 0, 0); }

    app->getDebugDrawPass()->record(commandList, app->getD3D12()->getWindowWidth(), app->getD3D12()->getWindowHeight(), view, proj);

}


bool Exercise4::createVertexBuffer()
{
    struct Vertex
    {
        Vector3 position;
        Vector2 uv;
        Vector4 color;
    };

    static Vertex vertices[24] =
    {
        // Front face (Blue)     
     { Vector3(-1.0f, -1.0f,  1.0f), Vector2(0.0f, 1.0f), Vector4(0.0f, 0.0f, 1.0f, 1.0f) },
     { Vector3(-1.0f,  1.0f,  1.0f), Vector2(0.0f, 0.0f), Vector4(0.0f, 0.0f, 1.0f, 1.0f) },
     { Vector3(1.0f,  1.0f,  1.0f), Vector2(1.0f, 0.0f), Vector4(0.0f, 0.0f, 1.0f, 1.0f) },
     { Vector3(1.0f, -1.0f,  1.0f), Vector2(1.0f, 1.0f), Vector4(0.0f, 0.0f, 1.0f, 1.0f) },

     // Back face (Red)
     { Vector3(1.0f, -1.0f, -1.0f), Vector2(0.0f, 1.0f), Vector4(1.0f, 0.0f, 0.0f, 1.0f) },
     { Vector3(1.0f,  1.0f, -1.0f), Vector2(0.0f, 0.0f), Vector4(1.0f, 0.0f, 0.0f, 1.0f) },
     { Vector3(-1.0f,  1.0f, -1.0f), Vector2(1.0f, 0.0f), Vector4(1.0f, 0.0f, 0.0f, 1.0f) },
     { Vector3(-1.0f, -1.0f, -1.0f), Vector2(1.0f, 1.0f), Vector4(1.0f, 0.0f, 0.0f, 1.0f) },

     // Left face (Green)
     { Vector3(-1.0f, -1.0f, -1.0f), Vector2(0.0f, 1.0f), Vector4(0.0f, 1.0f, 0.0f, 1.0f) },
     { Vector3(-1.0f,  1.0f, -1.0f), Vector2(0.0f, 0.0f), Vector4(0.0f, 1.0f, 0.0f, 1.0f) },
     { Vector3(-1.0f,  1.0f,  1.0f), Vector2(1.0f, 0.0f), Vector4(0.0f, 1.0f, 0.0f, 1.0f) },
     { Vector3(-1.0f, -1.0f,  1.0f), Vector2(1.0f, 1.0f), Vector4(0.0f, 1.0f, 0.0f, 1.0f) },

     // Right face (Yellow)
     { Vector3(1.0f, -1.0f,  1.0f), Vector2(0.0f, 1.0f), Vector4(1.0f, 1.0f, 0.0f, 1.0f) },
     { Vector3(1.0f,  1.0f,  1.0f), Vector2(0.0f, 0.0f), Vector4(1.0f, 1.0f, 0.0f, 1.0f) },
     { Vector3(1.0f,  1.0f, -1.0f), Vector2(1.0f, 0.0f), Vector4(1.0f, 1.0f, 0.0f, 1.0f) },
     { Vector3(1.0f, -1.0f, -1.0f), Vector2(1.0f, 1.0f), Vector4(1.0f, 1.0f, 0.0f, 1.0f) },

     // Top face (Magenta)
     { Vector3(-1.0f,  1.0f,  1.0f), Vector2(0.0f, 1.0f), Vector4(1.0f, 0.0f, 1.0f, 1.0f) },
     { Vector3(-1.0f,  1.0f, -1.0f), Vector2(0.0f, 0.0f), Vector4(1.0f, 0.0f, 1.0f, 1.0f) },
     { Vector3(1.0f,  1.0f, -1.0f), Vector2(1.0f, 0.0f), Vector4(1.0f, 0.0f, 1.0f, 1.0f) },
     { Vector3(1.0f,  1.0f,  1.0f), Vector2(1.0f, 1.0f), Vector4(1.0f, 0.0f, 1.0f, 1.0f) },

     // Bottom face (Cyan)
     { Vector3(-1.0f, -1.0f, -1.0f), Vector2(0.0f, 1.0f), Vector4(0.0f, 1.0f, 1.0f, 1.0f) },
     { Vector3(-1.0f, -1.0f,  1.0f), Vector2(0.0f, 0.0f), Vector4(0.0f, 1.0f, 1.0f, 1.0f) },
     { Vector3(1.0f, -1.0f,  1.0f), Vector2(1.0f, 0.0f), Vector4(0.0f, 1.0f, 1.0f, 1.0f) },
     { Vector3(1.0f, -1.0f, -1.0f), Vector2(1.0f, 1.0f), Vector4(0.0f, 1.0f, 1.0f, 1.0f) }
    };

    vertexBuffer = app->getResources()->createDefaultBuffer(vertices, sizeof(vertices), "Exercise4");

    vertexBufferView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
    vertexBufferView.StrideInBytes = sizeof(Vertex);
    vertexBufferView.SizeInBytes = sizeof(vertices);

    return true;
}

bool Exercise4::createIndexBuffer()
{
    // 36 índices = 6 caras * 2 triángulos * 3 vértices
    static uint16_t indices[36] =
    {
        // Front face (0,1,2,3)
        0, 1, 2,
        0, 2, 3,

        // Back face (4,5,6,7)
        4, 5, 6,
        4, 6, 7,

        // Left face (8,9,10,11)
        8,  9, 10,
        8, 10, 11,

        // Right face (12,13,14,15)
        12, 13, 14,
        12, 14, 15,

        // Top face (16,17,18,19)
        16, 17, 18,
        16, 18, 19,

        // Bottom face (20,21,22,23)
        20, 21, 22,
        20, 22, 23
    };

    indexBuffer = app->getResources()->createDefaultBuffer(indices, sizeof(indices), "Exercise4_Indices");

    indexBufferView.BufferLocation = indexBuffer->GetGPUVirtualAddress();
    indexBufferView.SizeInBytes = sizeof(indices);
    indexBufferView.Format = DXGI_FORMAT_R16_UINT;

    return true;
}

bool Exercise4::createRootSignature()
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

bool Exercise4::createPSO()
{
    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        {"MY_POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
        D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"TEXCOORD",    0, DXGI_FORMAT_R32G32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"COLOR",       0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 20, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
    };

    auto dataVS = DX::ReadData(L"Exercise4VS.cso");
    auto dataPS = DX::ReadData(L"Exercise4PS.cso");

    if (dataVS.empty() || dataPS.empty()) {
        Logger::Err("ERROR: VS or PS .cso is empty — check build output and paths");
        return false;
    }
    else
    {
        Logger::Log("Exercise4: VS Data & PS Data: OK");
    }

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { inputLayout, sizeof(inputLayout) / sizeof(D3D12_INPUT_ELEMENT_DESC) };  // the structure describing our input layout
    psoDesc.pRootSignature = rootSignature.Get();                                                   // the root signature that describes the input data this pso needs
    psoDesc.VS = { dataVS.data(), dataVS.size() };                                                  // structure describing where to find the vertex shader bytecode and how large it is
    psoDesc.PS = { dataPS.data(), dataPS.size() };                                                  // same as VS but for pixel shader
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;                         // type of topology we are drawing
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;                                             // format of the render target
    psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;                    
    psoDesc.SampleDesc = { 1, 0 };                                                                  // must be the same sample description as the swapchain and depth/stencil buffer
    psoDesc.SampleMask = 0xffffffff;                                                                // sample mask has to do with multi-sampling. 0xffffffff means point sampling is done
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);                               // a default rasterizer state.
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);                                         // a default blend state.
    psoDesc.NumRenderTargets = 1;                                                                   // we are only binding one render target
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);  

    // create the pso
    return SUCCEEDED(app->getD3D12()->getDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso)));
}

void Exercise4::ExerciseMenu()
{
    ImGui::Begin("Scene", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

    if (ImGui::CollapsingHeader("Model", ImGuiTreeNodeFlags_DefaultOpen)) 
    {
        // Position
        ImGui::Text("Position");
        ImGui::SameLine(85.0f);
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 60.0f);
        ImGui::DragFloat3("##Pos", &positionX, 0.1f, -500.0f, 500.0f, "%.3f");
        ImGui::SameLine();
        if (ImGui::Button("Reset##Pos", ImVec2(50, 0))) {
            positionX = positionY = positionZ = 0.0f;
        }

        // Rotation  
        ImGui::Text("Rotation");
        ImGui::SameLine(85.0f);
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 60.0f);
        ImGui::SliderFloat3("##Rot", &rotationX, 0.0f, 360.0f);
        ImGui::SameLine();
        if (ImGui::Button("Reset##Rot", ImVec2(50, 0))) 
        {
            rotationX = rotationY = rotationZ = 0.0f;
        }

        // Scale
        ImGui::Text("Scale");
        ImGui::SameLine(85.0f);
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 60.0f);
        ImGui::SliderFloat3("##Sca", &scaleX, 0.5f, 10.0f);
        ImGui::SameLine();
        if (ImGui::Button("Reset##Scale", ImVec2(50, 0))) 
        {
            scaleX = scaleY = scaleZ = 1.0f;
        }
    }

    if (ImGui::CollapsingHeader("View", ImGuiTreeNodeFlags_DefaultOpen)) 
    {
        ImGui::Text("Distance"); ImGui::SameLine(85.0f);
        ImGui::SliderFloat("##Dist", &camDistance, 1.0f, 100.0f, "%.1f");

        ImGui::Text("Height"); ImGui::SameLine(85.0f);
        ImGui::SliderFloat("##Height", &camHeight, -50.0f, 50.0f, "%.1f");

        ImGui::Text("Side"); ImGui::SameLine(85.0f);
        ImGui::SliderFloat("##Side", &camSide, -20.0f, 20.0f, "%.1f");
    }

    if (ImGui::CollapsingHeader("Projection", ImGuiTreeNodeFlags_DefaultOpen)) 
    {
        ImGui::Text("FOV"); ImGui::SameLine(85.0f);
        ImGui::SliderFloat("##Fov", &camFov, 0.1f, 3.0f, "%.2f");

        ImGui::Text("Near"); ImGui::SameLine(85.0f);
        ImGui::SliderFloat("##Near", &camNear, 0.01f, 5.0f, "%.3f");

        ImGui::Text("Far"); ImGui::SameLine(85.0f);
        ImGui::SliderFloat("##Far", &camFar, 10.0f, 500.0f, "%.0f");
    }
    if (ImGui::CollapsingHeader("Dsiplay", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Checkbox("Show grid", &isGridVisible);
        ImGui::SameLine();
        ImGui::Checkbox("Show axis", &isAxisVisible);
        ImGui::SameLine();
        ImGui::Checkbox("Show model", &isGeoVisible);
    }

    ImGui::Separator();
    if (ImGui::Button("Reset All", ImVec2(ImGui::GetContentRegionAvail().x, 0))) 
    {
        // Model resets
        positionX = positionY = positionZ = 0.0f;
        rotationX = rotationY = rotationZ = 0.0f;
        scaleX = scaleY = scaleZ = 1.0f;
        // Camera resets  
        camDistance = 5.0f;
        camHeight = 3.0f;
        camSide = 0.0f;
        camFov = XM_PIDIV4;
        camNear = 0.1f;
        camFar = 100.0f;
        //Display
        isGridVisible = true;
        isAxisVisible = true;
        isGeoVisible = true;
    }

    ImGui::End();
}

