#include "Globals.h"
#include "Exercise4.h"

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
#include "SceneRenderPass.h"

bool Exercise4::init()
{
    if (!createVertexBuffer()) 
    {
        Logger::Err("Exercise 4: VertexBuffer Failed");
        return false;
    }

    if (!createIndexBuffer()) 
    {
        Logger::Err("Exercise 4: IndexBuffer Failed");
        return false;
    }

    if (!createRootSignature()) 
    {
        Logger::Err("Exercise 4: RootSignature Failed");
        return false;
    }

    if (!createPSO()) 
    {
        Logger::Err("Exercise 4: PSO Failed");
        return false;
    }

    texture = app->getResources()->createTextureFromFile(L"Assets/Textures/dog.dds");
    ShaderDescriptorsModule* shaders = app->getShaderDescriptors();
    textureIndex = shaders->createSRV(texture.Get());

    if (texture) 
    {
        Logger::Log("TEXTURE OK!");
    }

    SamplersModule* samplers = app->getSamplers();

    D3D12_SAMPLER_DESC samp = {};
    samp.MinLOD = 0.0f;
    samp.MaxLOD = D3D12_FLOAT32_MAX;
    samp.MipLODBias = 0.0f;
    samp.MaxAnisotropy = 1;
    samp.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    samp.BorderColor[0] = samp.BorderColor[1] = samp.BorderColor[2] = samp.BorderColor[3] = 0.0f;

    // i) Wrap + Bilinear (slot 0)
    samp.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    samp.AddressU = samp.AddressV = samp.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samplerIndices[0] = samplers->createSampler(samp);

    // ii) Clamp + Bilinear (slot 1)
    samp.AddressU = samp.AddressV = samp.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    samplerIndices[1] = samplers->createSampler(samp);

    // iii) Wrap + Point (slot 2)
    samp.AddressU = samp.AddressV = samp.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samp.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
    samplerIndices[2] = samplers->createSampler(samp);

    // iv) Clamp + Point (slot 3)
    samp.AddressU = samp.AddressV = samp.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    samplerIndices[3] = samplers->createSampler(samp);

    return true;
}

void Exercise4::render()
{
    // ------------------------------------------------------------
    D3D12Module* d3d12 = app->getD3D12();
    ID3D12GraphicsCommandList* commandList = d3d12->getCommandList();
    CameraModule* camera = app->getCamera();

    // ----------------------------------------------------------------
    // ImGui Window
    // ----------------------------------------------------------------
    ExerciseMenu(camera);

    //Viewport
    SceneRenderPass pass = GetSceneRenderPass(app);

    float clearColor[] = { 0.2f, 0.2f, 0.2f, 1.0f };
    pass.begin(commandList, clearColor);
  

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


    SimpleMath::Matrix view = camera->getView();
    SimpleMath::Matrix proj = camera->GetProjection(pass.aspect);

    mvpMatrix = (model * view * proj).Transpose();

    commandList->SetGraphicsRoot32BitConstants(0, sizeof(Matrix) / sizeof(UINT32), &mvpMatrix, 0);

    // ------------------------------------------------------------
    // BIND TEXTURES
    // ------------------------------------------------------------
    ShaderDescriptorsModule* shaders = app->getShaderDescriptors();
    SamplersModule* samplers = app->getSamplers();

    ID3D12DescriptorHeap* heaps[] = {
        shaders->getHeap(),     // t0: texture
        samplers->getHeap()     // s0: sampler
    };
    commandList->SetDescriptorHeaps(2, heaps);  // 2 heaps

    // t0: texture
    D3D12_GPU_DESCRIPTOR_HANDLE texHandle = shaders->getGPUHandle(textureIndex);
    commandList->SetGraphicsRootDescriptorTable(1, texHandle);

    // s0: sampler samplerMode
    D3D12_GPU_DESCRIPTOR_HANDLE sampHandle = samplers->getGPUHandle((UINT)samplerMode);
    commandList->SetGraphicsRootDescriptorTable(2, sampHandle); 


    // ------------------------------------------------------------
    // GRID & AXIS
    // ------------------------------------------------------------
    if (isGridVisible) {dd::xzSquareGrid(-10.0f, 10.0f, 0.0f, 1.0f, dd::colors::LightGray);}
    if (isAxisVisible) { dd::axisTriad(ddConvert(SimpleMath::Matrix::Identity), 0.1f, 1.0f); }

    // ------------------------------------------------------------
    // Draw the geometry
    // ------------------------------------------------------------
    if (isGeoVisible) { commandList->DrawIndexedInstanced(36, 1, 0, 0, 0); }

    app->getDebugDrawPass()->record(commandList, pass.width, pass.height, view, proj);

    pass.end(commandList);
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
     { Vector3(-1.0f, -1.0f,  1.0f), Vector2(-0.5f, 2.5f), Vector4(0.0f, 0.0f, 1.0f, 1.0f) },
     { Vector3(-1.0f,  1.0f,  1.0f), Vector2(-0.5f, -0.5f), Vector4(0.0f, 0.0f, 1.0f, 1.0f) },
     { Vector3(1.0f,  1.0f,  1.0f), Vector2(2.5f, -0.5f), Vector4(0.0f, 0.0f, 1.0f, 1.0f) },
     { Vector3(1.0f, -1.0f,  1.0f), Vector2(2.5f, 2.5f), Vector4(0.0f, 0.0f, 1.0f, 1.0f) },

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
    // b0: 16 constants (float4x4 matrix for Vertex Shader)
    // t0: texture SRV descriptor table (Pixel Shader)
    // s0: sampler descriptor table (Pixel Shader)

    CD3DX12_ROOT_PARAMETER rootParameters[3];
    CD3DX12_DESCRIPTOR_RANGE srvRange;
    CD3DX12_DESCRIPTOR_RANGE sampRange;

    // ------------------------------------------------------------  
    // Root constants slot b0: 16x32-bit floats = float4x4 MVP matrix
    // ------------------------------------------------------------
    rootParameters[0].InitAsConstants(16, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);

    // ------------------------------------------------------------
   // SRV descriptor table slot t0: 1 texture SRV 
   // ------------------------------------------------------------
    srvRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); // 1 SRV, t0
    rootParameters[1].InitAsDescriptorTable(1, &srvRange, D3D12_SHADER_VISIBILITY_PIXEL);

    // ------------------------------------------------------------
    // Sampler descriptor table slot s0: 1 sampler
    // ------------------------------------------------------------
    sampRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0); // 1 sampler, s0
    rootParameters[2].InitAsDescriptorTable(1, &sampRange, D3D12_SHADER_VISIBILITY_PIXEL);

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

bool Exercise4::createPSO()
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
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);                                         // a default blend state.
    psoDesc.NumRenderTargets = 1;                                                                   // we are only binding one render target
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);  

    // ------------------------------------------------------------
    // Create Pipeline State Object
    // ------------------------------------------------------------
    return SUCCEEDED(app->getD3D12()->getDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso)));
}

void Exercise4::ExerciseMenu(CameraModule* camera)
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
        scaleX = scaleY = scaleZ = 1.0f;
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

