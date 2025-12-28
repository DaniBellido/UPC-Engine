#include "Globals.h"
#include "Exercise5.h"

#include "D3D12Module.h"
#include "ResourcesModule.h"
#include "ShaderDescriptorsModule.h"
#include "SamplersModule.h"
#include "CameraModule.h"
#include "Application.h"

#include <d3d12.h>
#include "ReadData.h"
#include <d3dcompiler.h>
#include "d3dx12.h"

#include "Model.h"
#include "Mesh.h"
#include "BasicMaterial.h"

Exercise5::Exercise5()
{
}

Exercise5::~Exercise5()
{
}

bool Exercise5::init()
{
    duck = std::make_unique<Model>();

    if (!duck->Load("D:/Development/MyRepository/UPC-Engine/Engine/Game/Assets/Models/Duck/Duck.gltf")) 
    {
        Logger::Err("Exercise5: Duck Model not loaded");
        return false;
    }

    if (!createRootSignature())
    {
        Logger::Err("Exercise 5: RootSignature Failed");
        return false;
    }

    if (!createPSO())
    {
        Logger::Err("Exercise 5: PSO Failed");
        return false;
    }

    return true;
}

void Exercise5::render()
{
    // ------------------------------------------------------------
    D3D12Module* d3d12 = app->getD3D12();
    ID3D12GraphicsCommandList* commandList = d3d12->getCommandList();
    CameraModule* camera = app->getCamera();

    // ----------------------------------------------------------------
    // ImGui Window
    // ----------------------------------------------------------------
    ExerciseMenu(camera);

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


    // ----------------------------------------------------------------
    // Model-View-Projection Matrix Pipeline
    // ----------------------------------------------------------------
    SimpleMath::Matrix rotX = SimpleMath::Matrix::CreateRotationX(DegreesToRadians(rotationX));
    SimpleMath::Matrix rotY = SimpleMath::Matrix::CreateRotationY(DegreesToRadians(rotationY));
    SimpleMath::Matrix rotZ = SimpleMath::Matrix::CreateRotationZ(DegreesToRadians(rotationZ));

    SimpleMath::Matrix scale = SimpleMath::Matrix::CreateScale(scaleX, scaleY, scaleZ);
    SimpleMath::Matrix position = SimpleMath::Matrix::CreateTranslation(positionX, positionY, positionZ);

    SimpleMath::Matrix model = scale * rotX * rotY * rotZ * position;


    SimpleMath::Matrix view = camera->getView();
    float aspect = camera->getAspect();
    SimpleMath::Matrix proj = camera->GetProjection(aspect);

    mvpMatrix = (model * view * proj).Transpose();
    commandList->SetGraphicsRoot32BitConstants(0, sizeof(Matrix) / sizeof(UINT32), &mvpMatrix, 0);
    duck->setModelMatrix(model);

    // ------------------------------------------------------------
    // BIND TEXTURES
    // ------------------------------------------------------------
    ShaderDescriptorsModule* shaders = app->getShaderDescriptors();
    SamplersModule* samplers = app->getSamplers();

    ID3D12DescriptorHeap* heaps[] = 
    {
        shaders->getHeap(),     // t0: texture
        samplers->getHeap()     // s0: sampler
    };
    commandList->SetDescriptorHeaps(2, heaps);  // 2 heaps


    // ------------------------------------------------------------
    // GRID & AXIS
    // ------------------------------------------------------------
    if (isGridVisible) { dd::xzSquareGrid(-10.0f, 10.0f, 0.0f, 1.0f, dd::colors::LightGray); }
    if (isAxisVisible) { dd::axisTriad(ddConvert(SimpleMath::Matrix::Identity), 0.1f, 1.0f); }

    // ------------------------------------------------------------
    // Draw Duck meshes
    // ------------------------------------------------------------
    if (isGeoVisible)
    {
        loadModel(commandList, shaders, samplers);
    }


    app->getDebugDrawPass()->record(commandList, app->getD3D12()->getWindowWidth(), app->getD3D12()->getWindowHeight(), view, proj);
}

bool Exercise5::createRootSignature()
{
    CD3DX12_ROOT_PARAMETER rootParameters[4];
    CD3DX12_DESCRIPTOR_RANGE srvRange;
    CD3DX12_DESCRIPTOR_RANGE sampRange;

    // ------------------------------------------------------------  
    // [0] MVP constants
    // ------------------------------------------------------------
    rootParameters[0].InitAsConstants(16, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);

    // ------------------------------------------------------------
    // [1] Materials CBV  (PS)
    // ------------------------------------------------------------
    rootParameters[1].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_PIXEL);

    // ------------------------------------------------------------
    // [2] SRV texture table (PS)
    // ------------------------------------------------------------
    srvRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
    rootParameters[2].InitAsDescriptorTable(1, &srvRange, D3D12_SHADER_VISIBILITY_PIXEL);

    // ------------------------------------------------------------
    // [3] Sampler Table (PS)
    // ------------------------------------------------------------
    sampRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0);
    rootParameters[3].InitAsDescriptorTable(1, &sampRange, D3D12_SHADER_VISIBILITY_PIXEL);

    // ------------------------------------------------------------
    // Create and serialize root signature
    // ------------------------------------------------------------
    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc(
        _countof(rootParameters), rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> signatureBlob;
    ComPtr<ID3DBlob> errorBlob;

    HRESULT hr = D3D12SerializeRootSignature(
        &rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);

    if (FAILED(hr))
    {
        if (errorBlob)
        {
            Logger::Err((char*)errorBlob->GetBufferPointer());
        }
        return false;
    }

    // ------------------------------------------------------------
    // Create root signature object
    // ------------------------------------------------------------
    hr = app->getD3D12()->getDevice()->CreateRootSignature(
        0,
        signatureBlob->GetBufferPointer(),
        signatureBlob->GetBufferSize(),
        IID_PPV_ARGS(&rootSignature)
    );

    if (FAILED(hr))
    {
        Logger::Err("CreateRootSignature failed");
        return false;
    }

    return true;
}

bool Exercise5::createPSO()
{
    // ------------------------------------------------------------
    // Input Layout: POSITION (vec3) + TEXCOORD (vec2)
    // ------------------------------------------------------------
    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
    };

    // ------------------------------------------------------------
    // Load compiled shaders (.cso files)
    // ------------------------------------------------------------
    auto dataVS = DX::ReadData(L"Exercise5VS.cso");
    auto dataPS = DX::ReadData(L"Exercise5PS.cso");

    if (dataVS.empty() || dataPS.empty()) {
        Logger::Err("ERROR: VS or PS .cso is empty — check build output and paths");
        return false;
    }
    else
    {
        Logger::Log("Exercise5: VS Data & PS Data: OK!");
    }

    // ------------------------------------------------------------
    // Pipeline State Object configuration
    // ------------------------------------------------------------
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
    psoDesc.RasterizerState.FrontCounterClockwise = TRUE;                                           // glTF winding order
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);                                         // a default blend state.
    psoDesc.NumRenderTargets = 1;                                                                   // we are only binding one render target
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

    // ------------------------------------------------------------
    // Create Pipeline State Object
    // ------------------------------------------------------------
    return SUCCEEDED(app->getD3D12()->getDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso)));
}

void Exercise5::loadModel(ID3D12GraphicsCommandList* commandList, ShaderDescriptorsModule* shaders, SamplersModule* samplers)
{
    for (size_t i = 0; i < duck->getMeshCount(); ++i)
    {
        const Mesh& mesh = duck->getMesh(i);

        // Vertex buffer
        const auto& vbv = mesh.getVertexView();
        commandList->IASetVertexBuffers(0, 1, &vbv);

        // Material
        const BasicMaterial& mat = duck->getMaterialForMesh(i);
        commandList->SetGraphicsRootConstantBufferView(1, mat.getMaterialBufferGPU());

        // Texture
        D3D12_GPU_DESCRIPTOR_HANDLE texHandle = shaders->getGPUHandle(mat.getColourTexSRV());
        commandList->SetGraphicsRootDescriptorTable(2, texHandle);

        // Sampler slot 3
        D3D12_GPU_DESCRIPTOR_HANDLE sampHandle = samplers->getGPUHandle(0);
        commandList->SetGraphicsRootDescriptorTable(3, sampHandle);

        // Draw
        if (mesh.hasIndices())
        {
            const auto& ibv = mesh.getIndexView();
            commandList->IASetIndexBuffer(&ibv);
            commandList->DrawIndexedInstanced(mesh.getIndexCount(), 1, 0, 0, 0);
        }
        else
        {
            commandList->DrawInstanced(mesh.getVertexCount(), 1, 0, 0);
        }
    }

}

void Exercise5::ExerciseMenu(CameraModule* camera)
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
        ImGui::SliderFloat3("##Sca", &scaleX, 0.01f, 1.0f);
        ImGui::SameLine();
        if (ImGui::Button("Reset##Scale", ImVec2(50, 0)))
        {
            scaleX = scaleY = scaleZ = 0.01f;
        }

    }

    if (ImGui::CollapsingHeader("Texture Filtering", ImGuiTreeNodeFlags_DefaultOpen))
    {
        const char* samplerNames[] = { "Wrap+Bilinear", "Clamp+Bilinear", "Wrap+Point", "Clamp+Point" };
        ImGui::Text("Sampler"); ImGui::SameLine(85.0f);
        ImGui::Combo("##Sampler", &samplerMode, samplerNames, IM_ARRAYSIZE(samplerNames));
    }

    if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen))
    {
        // Camera info
        ImGui::Text("Speed");
        ImGui::SameLine(120.0f);
        ImGui::Text("%.2f", camera->getSpeed());

        SimpleMath::Vector3 camPos = camera->getPos();
        ImGui::Text("Position");
        ImGui::SameLine(120.0f);
        ImGui::Text("(%.2f, %.2f, %.2f)", camPos.x, camPos.y, camPos.z);

        SimpleMath::Quaternion camRot = camera->getRot();
        ImGui::Text("Rotation");
        ImGui::SameLine(120.0f);
        ImGui::Text("(%.2f, %.2f, %.2f, %.2f)", camRot.x, camRot.y, camRot.z, camRot.w);

        ImGui::Separator();

        // Camera Projection
        if (ImGui::CollapsingHeader("Projection", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::PushItemWidth(180.0f);

            camFov = camera->GetFov();
            if (ImGui::SliderFloat("FOV", &camFov, 0.1f, 3.0f, "%.2f"))
                camera->SetFov(camFov);

            camNear = camera->GetNearPlane();
            if (ImGui::SliderFloat("Near", &camNear, 0.01f, 5.0f, "%.3f"))
                camera->SetNearPlane(camNear);

            camFar = camera->GetFarPlane();
            if (ImGui::SliderFloat("Far", &camFar, 10.0f, 500.0f, "%.0f"))
                camera->SetFarPlane(camFar);

            ImGui::PopItemWidth();
        }
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
        scaleX = scaleY = scaleZ = 0.01f;
        // Sampler reset
        samplerMode = 0;
        // Camera resets  
        camSpeed = 5.0f;
        camHeight = 3.0f;
        camSide = 0.0f;
        camFov = camera->SetFov(XM_PIDIV4);
        camNear = camera->SetNearPlane(0.1f);
        camFar = camera->SetFarPlane(100.0);
        //Display
        isGridVisible = true;
        isAxisVisible = true;
        isGeoVisible = true;
    }


    ImGui::End();
}




