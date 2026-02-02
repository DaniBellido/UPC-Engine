#include "Globals.h"
#include "Exercise8.h"

#include "D3D12Module.h"
#include "ResourcesModule.h"
#include "ShaderDescriptorsModule.h"
#include "SamplersModule.h"
#include "CameraModule.h"
#include "ViewportModule.h"
#include "RingBufferModule.h"
#include "Application.h"

#include <d3d12.h>
#include "ReadData.h"
#include <d3dcompiler.h>
#include "d3dx12.h"

#include "Model.h"
#include "Mesh.h"
#include "BasicMaterial.h"

#include "SceneRenderPass.h"

Exercise8::Exercise8()
{
}

Exercise8::~Exercise8()
{
}

bool Exercise8::init()
{
    duck = std::make_unique<Model>();

    qRot = SimpleMath::Quaternion::CreateFromYawPitchRoll(
            XMConvertToRadians(rotationY), // yaw   (Y)
            XMConvertToRadians(rotationX), // pitch (X)
            XMConvertToRadians(rotationZ)  // roll  (Z)
        );

    if (!duck->Load("Assets/Models/DamagedHelmet/", "damagedHelmet.gltf", BasicMaterial::Type::PBR_PHONG))
    {
        Logger::Err("Exercise8: Duck Model not loaded");
        return false;
    }

    if (!createRootSignature())
    {
        Logger::Err("Exercise 8: RootSignature Failed");
        return false;
    }

    if (!createPSO())
    {
        Logger::Err("Exercise 8: PSO Failed");
        return false;
    }

    return true;
}

void Exercise8::render()
{
    // ------------------------------------------------------------
    // Frame context
    // ------------------------------------------------------------
    D3D12Module* d3d12 = app->getD3D12();
    ID3D12GraphicsCommandList* commandList = d3d12->getCommandList();
    CameraModule* camera = app->getCamera();
    RingBufferModule* ring = app->getRingBuffer();
    ShaderDescriptorsModule* shaders = app->getShaderDescriptors();
    SamplersModule* samplers = app->getSamplers();

    // ----------------------------------------------------------------
    // ImGui Window
    // ----------------------------------------------------------------
    ExerciseMenu(camera);

    // ------------------------------------------------------------
    // Scene render pass (Viewport or Backbuffer)
    // ------------------------------------------------------------
    SceneRenderPass pass = GetSceneRenderPass(app);

    const float clearColor[] = { 0.2f, 0.2f, 0.2f, 1.0f };
    pass.begin(commandList, clearColor);

    // ------------------------------------------------------------
    // Pipeline state
    // ------------------------------------------------------------
    commandList->SetGraphicsRootSignature(rootSignature.Get()); // The root signature defines how resources are passed to shaders
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST); // Set primitive type (triangles)

    // ------------------------------------------------------------
    // Descriptor heaps
    // ------------------------------------------------------------
    ID3D12DescriptorHeap* heaps[] =
    {
        shaders->getHeap(),     // t0: texture
        samplers->getHeap()     // s0: sampler
    };
    commandList->SetDescriptorHeaps(2, heaps);  // 2 heaps

    // ------------------------------------------------------------
    // PerFrame constant buffer (Phong lighting)
    // ------------------------------------------------------------
    PerFrame* perFrame = nullptr;
    auto perFrameGPU = ring->allocBuffer(sizeof(PerFrame), (void**)&perFrame);

    perFrame->Ac = ambient;
    perFrame->viewPos = camera->getPos();

    perFrame->NumDirLights = 1;
    perFrame->NumPointLights = 1;
    perFrame->NumSpotLights = 1;

    commandList->SetGraphicsRootConstantBufferView(2, perFrameGPU);

    // Directional
    DirectionalLightGPU dirLights[1] = {};
    dirLights[0].direction = XMFLOAT3(lightDir.x, lightDir.y, lightDir.z);
    dirLights[0].color = XMFLOAT3(lightColor.x, lightColor.y, lightColor.z);
    dirLights[0].intensity = 1.0f;

    // Point
    PointLightGPU pointLights[1] = {};
    pointLights[0].position = XMFLOAT3(pointPosition.x, pointPosition.y, pointPosition.z);
    pointLights[0].color = XMFLOAT3(pointColor.x, pointColor.y, pointColor.z);
    pointLights[0].intensity = pointIntensity;
    pointLights[0].radius = pointRange;


    // Spot
    spotDirection.Normalize();
    if (spotOuterAngleDeg < spotInnerAngleDeg) spotOuterAngleDeg = spotInnerAngleDeg;

    float innerRad = XMConvertToRadians(spotInnerAngleDeg);
    float outerRad = XMConvertToRadians(spotOuterAngleDeg);

    SpotLightGPU spotLights[1] = {};
    spotLights[0].position = XMFLOAT3(spotPosition.x, spotPosition.y, spotPosition.z);
    spotLights[0].direction = XMFLOAT3(spotDirection.x, spotDirection.y, spotDirection.z);
    spotLights[0].color = XMFLOAT3(spotColor.x, spotColor.y, spotColor.z);
    spotLights[0].intensity = spotIntensity;
    spotLights[0].radius = spotRange;
    spotLights[0].cosInnerAngle = cosf(innerRad);
    spotLights[0].cosOuterAngle = cosf(outerRad);

    // Upload DirLights
    DirectionalLightGPU* dirCPU = nullptr;
    auto dirGPU = ring->allocBuffer(sizeof(dirLights), (void**)&dirCPU);
    memcpy(dirCPU, dirLights, sizeof(dirLights));

    // Upload PointLights
    PointLightGPU* pointCPU = nullptr;
    auto pointGPU = ring->allocBuffer(sizeof(pointLights), (void**)&pointCPU);
    memcpy(pointCPU, pointLights, sizeof(pointLights));

    // Upload SpotLights
    SpotLightGPU* spotCPU = nullptr;
    auto spotGPU = ring->allocBuffer(sizeof(spotLights), (void**)&spotCPU);
    memcpy(spotCPU, spotLights, sizeof(spotLights));

    commandList->SetGraphicsRootShaderResourceView(3, dirGPU);
    commandList->SetGraphicsRootShaderResourceView(4, pointGPU);
    commandList->SetGraphicsRootShaderResourceView(5, spotGPU);

    // ----------------------------------------------------------------
    // Model-View-Projection Matrix
    // ----------------------------------------------------------------
    SimpleMath::Matrix model =
        SimpleMath::Matrix::CreateScale(scaleX, scaleY, scaleZ) *
        SimpleMath::Matrix::CreateFromQuaternion(qRot) *
        SimpleMath::Matrix::CreateTranslation(positionX, positionY, positionZ);

    duck->setModelMatrix(model);

    if (isGizmoVisible)
        ApplyImGuizmo(camera);

    auto proj = camera->GetProjection(pass.aspect);
    mvpMatrix = (duck->getModelMatrix() * camera->getView() * proj).Transpose();

    commandList->SetGraphicsRoot32BitConstants(0, sizeof(Matrix) / sizeof(UINT32), &mvpMatrix, 0);

    // ------------------------------------------------------------
    // DEBUG DRAWS
    // ------------------------------------------------------------
    if (isPointLightGizmoVisible) 
    {
       // --- Point light debug ---
       // Small sphere = point position
        dd::sphere(ddConvert(pointPosition), dd::colors::White, 0.15f, 0, true);

        // Big sphere = point range
        dd::sphere(ddConvert(pointPosition), dd::colors::Yellow, pointRange, 0, true);
    }

    if (isSpotGizmoVisible)
    {
        SimpleMath::Vector3 dir = spotDirection;
        dir.Normalize();

        // outer angle in radians
        float outerRad = XMConvertToRadians(spotOuterAngleDeg);

        // cone base radius at the end of the range
        float baseRadius = tanf(outerRad) * spotRange;

        // draw cone (apex at light position)
        dd::cone(
            ddConvert(spotPosition),
            ddConvert(dir),
            dd::colors::Yellow,
            baseRadius,
            0.0f,   // apexRadius (0 = sharp cone)
            0,      // durationMillis
            true    // depthEnabled
        );

        float innerRad = XMConvertToRadians(spotInnerAngleDeg);
        float innerBaseRadius = tanf(innerRad) * spotRange;

        dd::cone(
            ddConvert(spotPosition),
            ddConvert(dir),
            dd::colors::GreenYellow,
            innerBaseRadius,
            0.0f,
            0,
            true
        );

        // optional: small sphere at the light position
        dd::sphere(ddConvert(spotPosition), dd::colors::White, 0.15f, 0, true);
    }

    if (isLightGizmoVisible)
    {
        // --- Directional light debug ---
        // Choose a reference position (origin is fine)
        SimpleMath::Vector3 origin(0.0f, 0.0f, 0.0f);

        // Directional light points *from* light *towards* the scene
        SimpleMath::Vector3 dir = -lightDir;
        dir.Normalize();

        float length = 2.5f;
        SimpleMath::Vector3 end = origin + dir * length;

        dd::arrow(ddConvert(origin), ddConvert(end), dd::colors::Yellow, 0.05f);

    }

    if (isGridVisible) { dd::xzSquareGrid(-10.0f, 10.0f, 0.0f, 1.0f, dd::colors::LightGray); }
    if (isAxisVisible) { dd::axisTriad(ddConvert(SimpleMath::Matrix::Identity), 0.1f, 1.0f); }

    // ------------------------------------------------------------
    // Draw geometry 
    // ------------------------------------------------------------
    if (isGeoVisible)
    {
        // ---------- Base pass ----------
        if (!isWireframe)
        {
            if (isNormalsVisible)
            {
                commandList->SetPipelineState(psoNormals.Get());
                drawModel(commandList, shaders, samplers);
            }
            else
            {
                commandList->SetPipelineState(pso.Get());
                drawModel(commandList, shaders, samplers);
            }
        }

        // ---------- Wireframe pass ----------
        if (isWireframe || isWireframeOverlay)
        {
            commandList->SetPipelineState(psoWireframe.Get());
            drawModel(commandList, shaders, samplers);
        }
    }

    // ------------------------------------------------------------
    // Debug pass (last)
    // ------------------------------------------------------------
    app->getDebugDrawPass()->record(commandList, pass.width, pass.height, camera->getView(), proj);

    pass.end(commandList);
}


bool Exercise8::createRootSignature()
{
    CD3DX12_ROOT_PARAMETER rootParameters[8];
    CD3DX12_DESCRIPTOR_RANGE srvRange;
    CD3DX12_DESCRIPTOR_RANGE sampRange;

    // ------------------------------------------------------------  
    // [0] MVP constants b0
    // ------------------------------------------------------------
    rootParameters[0].InitAsConstants(16, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);

    // ------------------------------------------------------------
    // [1] PerInstance CBV (b1) - VS + PS
    // ------------------------------------------------------------
    rootParameters[1].InitAsConstantBufferView(1, 0, D3D12_SHADER_VISIBILITY_ALL);

    // ------------------------------------------------------------
    // [2] PerFrame CBV (b2) - PS
    // ------------------------------------------------------------
    rootParameters[2].InitAsConstantBufferView(2, 0, D3D12_SHADER_VISIBILITY_PIXEL);

    // [3] DirLights root SRV (t0)
    rootParameters[3].InitAsShaderResourceView(0, 0, D3D12_SHADER_VISIBILITY_PIXEL);

    // [4] PointLights root SRV (t1)
    rootParameters[4].InitAsShaderResourceView(1, 0, D3D12_SHADER_VISIBILITY_PIXEL);

    // [5] SpotLights root SRV (t2)
    rootParameters[5].InitAsShaderResourceView(2, 0, D3D12_SHADER_VISIBILITY_PIXEL);

    // ------------------------------------------------------------
    // [6]  SRV texture table (t3) - PS
    // ------------------------------------------------------------
    srvRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 3);
    rootParameters[6].InitAsDescriptorTable(1, &srvRange, D3D12_SHADER_VISIBILITY_PIXEL);

    // ------------------------------------------------------------
    // [4] Sampler Table (s0) - PS
    // ------------------------------------------------------------
    sampRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0);
    rootParameters[7].InitAsDescriptorTable(1, &sampRange, D3D12_SHADER_VISIBILITY_PIXEL);

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

bool Exercise8::createPSO()
{
    // ------------------------------------------------------------
    // Input Layout: POSITION (vec3) + TEXCOORD (vec2)
    // ------------------------------------------------------------
    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}

    };

    // ------------------------------------------------------------
    // Load compiled shaders (.cso files)
    // ------------------------------------------------------------
    auto dataVS = DX::ReadData(L"Exercise8VS.cso");
    auto dataPS = DX::ReadData(L"Exercise8PS.cso");


    if (dataVS.empty() || dataPS.empty()) {
        Logger::Err("ERROR: VS or PS .cso is empty — check build output and paths");
        return false;
    }
    else
    {
        Logger::Log("Exercise8: VS Data & PS Data: OK!");
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
    // Create Solid PSO
    // ------------------------------------------------------------
    HRESULT hr = app->getD3D12()->getDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso));

    if (FAILED(hr))
    {
        Logger::Err("Failed to create solid PSO");
        return false;
    }

    // ------------------------------------------------------------
    // Create WIREFRAME PSO (overlay-ready)
    // ------------------------------------------------------------
    D3D12_GRAPHICS_PIPELINE_STATE_DESC wireDesc = psoDesc;

    auto wireframePS = DX::ReadData(L"WireframePS.cso");

    wireDesc.PS = { wireframePS.data(), wireframePS.size() };

    wireDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
    wireDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

    wireDesc.RasterizerState.DepthBias = 500;
    wireDesc.RasterizerState.SlopeScaledDepthBias = -4.0f;
    wireDesc.RasterizerState.DepthBiasClamp = 0.0f;

    wireDesc.DepthStencilState.DepthWriteMask =
        D3D12_DEPTH_WRITE_MASK_ZERO;

    hr = app->getD3D12()->getDevice()->CreateGraphicsPipelineState(
        &wireDesc, IID_PPV_ARGS(&psoWireframe));

    if (FAILED(hr))
    {
        Logger::Err("Exercise6: Failed to create wireframe PSO");
        return false;
    }

    // ------------------------------------------------------------
    // Create NORMALS PSO
    // ------------------------------------------------------------
    auto dataNormalsPS = DX::ReadData(L"NormalsPS.cso");

    D3D12_GRAPHICS_PIPELINE_STATE_DESC normalsDesc = psoDesc;
    normalsDesc.PS = { dataNormalsPS.data(), dataNormalsPS.size() };

    HRESULT _hr = app->getD3D12()->getDevice()->CreateGraphicsPipelineState(&normalsDesc, IID_PPV_ARGS(&psoNormals));

    if (FAILED(_hr))
    {
        Logger::Err("Failed to create normals PSO");
        return false;
    }

    return true;
}

void Exercise8::drawModel(ID3D12GraphicsCommandList* commandList, ShaderDescriptorsModule* shaders, SamplersModule* samplers)
{
    RingBufferModule* ring = app->getRingBuffer();

    const SimpleMath::Matrix modelMat = duck->getModelMatrix();
    const SimpleMath::Matrix normalMat = modelMat.Invert().Transpose();

    for (size_t i = 0; i < duck->getMeshCount(); ++i)
    {
        // ------------------------------------------------------------
        // Mesh geometry
        // ------------------------------------------------------------
        const Mesh& mesh = duck->getMesh(i);

        // Vertex buffer
        const D3D12_VERTEX_BUFFER_VIEW& vbv = mesh.getVertexView();
        commandList->IASetVertexBuffers(0, 1, &vbv);

        if (mesh.hasIndices())
        {
            const D3D12_INDEX_BUFFER_VIEW& ibv = mesh.getIndexView();
            commandList->IASetIndexBuffer(&ibv);
        }

        // ------------------------------------------------------------
        // Per-instance constants (transform + phong material)
        // ------------------------------------------------------------
        PerInstance* perInstance = nullptr;
        auto perInstanceGPU = ring->allocBuffer(sizeof(PerInstance), (void**)&perInstance);

        perInstance->modelMat = duck->getModelMatrix().Transpose();
        perInstance->normalMat = duck->getModelMatrix().Invert();

        // Material data
        const BasicMaterial& mat = duck->getMaterialForMesh(i);

        perInstance->material = mat.getPBRPhongMaterial();

        // ImGui overrides (PBR-friendly)
        perInstance->material.diffuseColour = XMFLOAT3(
            pbrPhongDiffuseColor.x,
            pbrPhongDiffuseColor.y,
            pbrPhongDiffuseColor.z
        );

        perInstance->material.specularColour = XMFLOAT3(
            pbrPhongSpecularColor.x,
            pbrPhongSpecularColor.y,
            pbrPhongSpecularColor.z
        );

        perInstance->material.hasDiffuseTex =
            isTextureVisible && mat.getPBRPhongMaterial().hasDiffuseTex;

        perInstance->material.shininess = pbrPhongShininess;

        commandList->SetGraphicsRootConstantBufferView(1, perInstanceGPU); // Bind PerInstance CBV -> slot 1 (b1)

        // ------------------------------------------------------------
        // Material resources (texture + sampler)
        // ------------------------------------------------------------
        D3D12_GPU_DESCRIPTOR_HANDLE texHandle = shaders->getGPUHandle(mat.getColourTexSRV()); // Texture (t0)
        commandList->SetGraphicsRootDescriptorTable(6, texHandle);

        D3D12_GPU_DESCRIPTOR_HANDLE sampHandle = samplers->getGPUHandle(0);  // Sampler (s0)
        commandList->SetGraphicsRootDescriptorTable(7, sampHandle);

        // ------------------------------------------------------------
        // Draw
        // ------------------------------------------------------------
        if (mesh.hasIndices())
        {
            commandList->DrawIndexedInstanced(mesh.getIndexCount(), 1, 0, 0, 0);
        }
        else
        {
            commandList->DrawInstanced(mesh.getVertexCount(), 1, 0, 0);
        }
    }
}

void Exercise8::ApplyImGuizmo(CameraModule* camera)
{
    ViewportModule* vp = app->getViewport();
    if (!vp || !vp->isVisible())
        return;

    if (!vp->isHovered() && !vp->isFocused() && !ImGuizmo::IsUsing())
        return;

    ImGuizmo::BeginFrame();
    ImGuizmo::Enable(true);
    ImDrawList* dl = vp->getDrawList();
    if (!dl) return;
    ImGuizmo::SetDrawlist(dl);

    ImGui::Begin("Viewport");
    ImGuizmo::BeginFrame();
    ImGuizmo::Enable(true);
    ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());

    ImVec2 pos = vp->getViewportPos();
    ImVec2 size = vp->getViewportSize();
    ImGuizmo::SetRect(pos.x, pos.y, size.x, size.y);

    ImGuizmo::SetOrthographic(false);
    ImGuizmo::SetGizmoSizeClipSpace(0.1f);

    // -------------------------------
    // ImGuizmo Matrix
    // -------------------------------
    DirectX::XMMATRIX modelM = duck->getModelMatrix();
    DirectX::XMMATRIX viewM = camera->getView();

    float aspect = camera->getAspect();
    if (size.y > 0.0f)
        aspect = size.x / size.y;

    DirectX::XMMATRIX projM = camera->GetProjection(aspect);

    // XMFLOAT4X4 as intermediate buffer
    DirectX::XMFLOAT4X4 modelF;
    DirectX::XMFLOAT4X4 viewF;
    DirectX::XMFLOAT4X4 projF;

    DirectX::XMStoreFloat4x4(&modelF, modelM);
    DirectX::XMStoreFloat4x4(&viewF, viewM);
    DirectX::XMStoreFloat4x4(&projF, projM);

    // ImGuizmo Call
    ImGuizmo::Manipulate(&viewF.m[0][0], &projF.m[0][0],
        currentOperation, ImGuizmo::WORLD,
        &modelF.m[0][0]);

    ImGui::End();

    // -------------------------------
    // Apply changes to duck
    // -------------------------------
    if (ImGuizmo::IsUsing())
    {
        DirectX::XMMATRIX newModel = DirectX::XMLoadFloat4x4(&modelF);
        duck->setModelMatrix(SimpleMath::Matrix(newModel));

        // Actualiza quaternion interno
        SimpleMath::Vector3 scale, pos3;
        SimpleMath::Quaternion rot;
        SimpleMath::Matrix(newModel).Decompose(scale, rot, pos3);

        qRot = rot;
        scaleX = scale.x;
        scaleY = scale.y;
        scaleZ = scale.z;

        positionX = pos3.x;
        positionY = pos3.y;
        positionZ = pos3.z;

        SimpleMath::Vector3 euler = rot.ToEuler();
        rotationX = XMConvertToDegrees(euler.x);
        rotationY = XMConvertToDegrees(euler.y);
        rotationZ = XMConvertToDegrees(euler.z);
    }
}

void Exercise8::applyMaterialPreset(MaterialPreset preset)
{
    currentPreset = preset;

    switch (preset)
    {
    case MaterialPreset::Matte:
        pbrPhongDiffuseColor = { 0.8f, 0.8f, 0.8f, 1.0f };
        pbrPhongSpecularColor = { 0.04f, 0.04f, 0.04f };
        pbrPhongShininess = 2.0f;
        break;

    case MaterialPreset::Plastic:
        pbrPhongDiffuseColor = { 0.9f, 0.9f, 0.9f, 1.0f };
        pbrPhongSpecularColor = { 0.04f, 0.04f, 0.04f };
        pbrPhongShininess = 32.0f;
        break;

    case MaterialPreset::Metal:
        pbrPhongDiffuseColor = { 0.05f, 0.05f, 0.05f, 1.0f };
        pbrPhongSpecularColor = { 1.0f, 0.92f, 0.67f };
        pbrPhongShininess = 96.0f;
        break;

    case MaterialPreset::Rubber:
        pbrPhongDiffuseColor = { 0.28f, 0.28f, 0.28f, 1.0f };
        pbrPhongSpecularColor = { 0.02f, 0.02f, 0.02f };
        pbrPhongShininess = 8.0f;
        break;

    case MaterialPreset::Custom:
    default:
        pbrPhongDiffuseColor = { 0.28f, 0.28f, 0.28f, 1.0f };
        pbrPhongSpecularColor = { 0.015f, 0.015f, 0.015f };
        pbrPhongShininess = 64.0f;
        break;
    }
}

void Exercise8::ExerciseMenu(CameraModule* camera)
{
    ImGui::Begin("Scene", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

    ImGui::Separator();
    ImGui::Text("ImGuizmo: Over=%d Using=%d",
        (int)ImGuizmo::IsOver(),
        (int)ImGuizmo::IsUsing());

    if (ImGui::CollapsingHeader("Model Transform", ImGuiTreeNodeFlags_DefaultOpen))
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
        if (ImGui::SliderFloat3("##Rot", &rotationX, 0.0f, 360.0f))
        {
            float pitch = XMConvertToRadians(rotationX);
            float yaw = XMConvertToRadians(rotationY);
            float roll = XMConvertToRadians(rotationZ);

            qRot = SimpleMath::Quaternion::CreateFromYawPitchRoll(yaw, pitch, roll);
        }

        ImGui::SameLine();
        if (ImGui::Button("Reset##Rot", ImVec2(50, 0)))
        {
            rotationX = 90.0f;
            rotationY = rotationZ = 0.0f;
            qRot = SimpleMath::Quaternion::CreateFromYawPitchRoll(
                XMConvertToRadians(rotationY), // yaw   (Y)
                XMConvertToRadians(rotationX), // pitch (X)
                XMConvertToRadians(rotationZ)  // roll  (Z);
            );
        }

        // Scale
        ImGui::Text("Scale");
        ImGui::SameLine(85.0f);
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 60.0f);
        ImGui::SliderFloat3("##Sca", &scaleX, 0.01f, 5.0f);
        ImGui::SameLine();
        if (ImGui::Button("Reset##Scale", ImVec2(50, 0)))
        {
            scaleX = scaleY = scaleZ = 1.00f;
        }

        ImGui::Separator();

        if (ImGui::RadioButton("Translate [Z]", currentOperation == ImGuizmo::TRANSLATE) || ImGui::IsKeyPressed(ImGuiKey_Z))
            currentOperation = ImGuizmo::TRANSLATE;
        ImGui::SameLine(0, 10);

        if (ImGui::RadioButton("Rotate [X]", currentOperation == ImGuizmo::ROTATE) || ImGui::IsKeyPressed(ImGuiKey_X))
            currentOperation = ImGuizmo::ROTATE;
        ImGui::SameLine(0, 10);

        if (ImGui::RadioButton("Scale [C]", currentOperation == ImGuizmo::SCALE) || ImGui::IsKeyPressed(ImGuiKey_C))
            currentOperation = ImGuizmo::SCALE;

        ImGui::Separator();

        ImGui::Checkbox("Show gizmo", &isGizmoVisible);
        ImGui::SameLine();
        ImGui::Checkbox("Show model", &isGeoVisible);


    }

    if (ImGui::CollapsingHeader("PBR-Phong Material", ImGuiTreeNodeFlags_DefaultOpen))
    {
        static int presetIndex = 0;
        const char* presets[] = { "Custom", "Matte", "Plastic", "Metal", "Rubber" };

        ImGui::Text("Material Preset");
        ImGui::SameLine(125.0f);
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 60.0f);
        if (ImGui::Combo("##MatPre", &presetIndex, presets, IM_ARRAYSIZE(presets)))
        {
            applyMaterialPreset(static_cast<MaterialPreset>(presetIndex));
        }

        ImGui::Text("Diffuse (Albedo)");
        ImGui::SameLine(125.0f);
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 60.0f);
        ImGui::ColorEdit4("##DiffCol", &pbrPhongDiffuseColor.x);

        ImGui::Text("Specular (F0)");
        ImGui::SameLine(125.0f);
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 60.0f);
        ImGui::ColorEdit4("##SpecCol", &pbrPhongSpecularColor.x);

        ImGui::Text("Shininess");
        ImGui::SameLine(125.0f);
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 60.0f);
        ImGui::SliderFloat("##Shin", &pbrPhongShininess, 1.0f, 128.0f);


        ImGui::Separator();

        ImGui::Checkbox("Use Texture", &isTextureVisible);
        ImGui::SameLine();
        ImGui::Checkbox("Show normals", &isNormalsVisible);

        ImGui::Separator();

        // -------------------------
        // Wireframe modes 
        // -------------------------
        enum class WireMode { Solid, Wireframe, Overlay };
        static WireMode wireMode = WireMode::Solid;

        ImGui::Text("View Mode");
        ImGui::RadioButton("Solid", (int*)&wireMode, (int)WireMode::Solid);
        ImGui::SameLine(0, 10);
        ImGui::RadioButton("Wireframe", (int*)&wireMode, (int)WireMode::Wireframe);
        ImGui::SameLine(0, 10);
        ImGui::RadioButton("Overlay", (int*)&wireMode, (int)WireMode::Overlay);

        isWireframe = (wireMode == WireMode::Wireframe);
        isWireframeOverlay = (wireMode == WireMode::Overlay);
    }

    if (ImGui::CollapsingHeader("Lighting", ImGuiTreeNodeFlags_DefaultOpen))
    {
        // DIRECTIONAL LIGHT
        ImGui::Text("Directial Light");

        ImGui::Text("Light Direction");
        ImGui::SameLine(125.0f);
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 60.0f);
        ImGui::DragFloat3("##LightDir", &lightDir.x, 0.01f, -1.0f, 1.0f);
        lightDir.Normalize();

        ImGui::Text("Light Color");
        ImGui::SameLine(125.0f);
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 60.0f);
        ImGui::ColorEdit3("##LightCol", &lightColor.x);

        ImGui::Text("Ambient");
        ImGui::SameLine(125.0f);
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 60.0f);
        ImGui::ColorEdit3("##Ambi", &ambient.x);

        ImGui::Separator();
        ImGui::Checkbox("Show Light Direction", &isLightGizmoVisible);

        // POINT LIGHT
        ImGui::Separator();
        ImGui::Text("Point Light");

        ImGui::Text("Position");
        ImGui::SameLine(125.0f);
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 60.0f);
        ImGui::DragFloat3("##PointPos", &pointPosition.x, 0.05f, -50.0f, 50.0f);

        ImGui::Text("Range");
        ImGui::SameLine(125.0f);
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 60.0f);
        ImGui::SliderFloat("##PointRange", &pointRange, 0.1f, 50.0f, "%.2f");
        
        pointRange = std::max(pointRange, 0.01f);

        ImGui::Text("Intensity");
        ImGui::SameLine(125.0f);
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 60.0f);
        ImGui::SliderFloat("##PointIntensity", &pointIntensity, 0.0f, 100.0f, "%.2f");

        ImGui::Text("Color");
        ImGui::SameLine(125.0f);
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 60.0f);
        ImGui::ColorEdit3("##PointColor", &pointColor.x);

        ImGui::Checkbox("Show Point Light", &isPointLightGizmoVisible);


        //SPOT LIGHT
        ImGui::Separator();
        ImGui::Text("Spot Light");

        // Enable/disable gizmo
        ImGui::Checkbox("Show Spot Gizmo", &isSpotGizmoVisible);

        // Position
        ImGui::Text("Position");
        ImGui::SameLine(125.0f);
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 60.0f);
        ImGui::DragFloat3("##SpotPos", &spotPosition.x, 0.05f, -50.0f, 50.0f);

        // Direction (normalized)
        ImGui::Text("Direction");
        ImGui::SameLine(125.0f);
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 60.0f);
        ImGui::DragFloat3("##SpotDir", &spotDirection.x, 0.01f, -1.0f, 1.0f);
        spotDirection.Normalize();

        // Range
        ImGui::Text("Range");
        ImGui::SameLine(125.0f);
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 60.0f);
        ImGui::SliderFloat("##SpotRange", &spotRange, 0.1f, 100.0f, "%.2f");
        spotRange = std::max(spotRange, 0.01f);

        // Intensity
        ImGui::Text("Intensity");
        ImGui::SameLine(125.0f);
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 60.0f);
        ImGui::SliderFloat("##SpotIntensity", &spotIntensity, 0.0f, 200.0f, "%.2f");

        // Color
        ImGui::Text("Color");
        ImGui::SameLine(125.0f);
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 60.0f);
        ImGui::ColorEdit3("##SpotColor", &spotColor.x);

        // Inner / Outer angles
        ImGui::Text("Inner Angle");
        ImGui::SameLine(125.0f);
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 60.0f);
        ImGui::SliderFloat("##SpotInner", &spotInnerAngleDeg, 0.1f, 89.0f, "%.1f deg");

        ImGui::Text("Outer Angle");
        ImGui::SameLine(125.0f);
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 60.0f);
        ImGui::SliderFloat("##SpotOuter", &spotOuterAngleDeg, 0.1f, 89.0f, "%.1f deg");

        // Keep angles sane: outer >= inner and avoid tan(90)
        spotInnerAngleDeg = std::min(spotInnerAngleDeg, 89.0f);
        spotOuterAngleDeg = std::min(spotOuterAngleDeg, 89.0f);
        if (spotOuterAngleDeg < spotInnerAngleDeg)
            spotOuterAngleDeg = spotInnerAngleDeg;

        // Optional quick reset button
        if (ImGui::Button("Reset Spot"))
        {
            spotPosition = { 0.0f, 5.0f, 0.0f };
            spotDirection = { 0.0f, -1.0f, 0.0f };
            spotRange = 20.0f;
            spotIntensity = 30.0f;
            spotColor = { 1.0f, 1.0f, 1.0f };
            spotInnerAngleDeg = 15.0f;
            spotOuterAngleDeg = 25.0f;
        }
    }

    if (ImGui::CollapsingHeader("Camera"))
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

        ImGui::Text("Projection");
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

    if (ImGui::CollapsingHeader("World"))
    {
        ImGui::Checkbox("Show grid    ", &isGridVisible);
        ImGui::SameLine();
        ImGui::Checkbox("Show axis", &isAxisVisible);


    }

    ImGui::Separator();
    if (ImGui::Button("Reset All", ImVec2(ImGui::GetContentRegionAvail().x, 0)))
    {
        // Model resets
        positionX = positionY = positionZ = 0.0f;
        rotationX = 90.0f;
        rotationY = rotationZ = 0.0f;
        //qRot = SimpleMath::Quaternion::Identity;
        qRot = SimpleMath::Quaternion::CreateFromYawPitchRoll(
            XMConvertToRadians(rotationY), // yaw   (Y)
            XMConvertToRadians(rotationX), // pitch (X)
            XMConvertToRadians(rotationZ)  // roll  (Z);
        );
        scaleX = scaleY = scaleZ = 1.0f;

        // Camera resets  
        camSpeed = 5.0f;
        camFov = camera->SetFov(XM_PIDIV4);
        camNear = camera->SetNearPlane(1.0f);
        camFar = camera->SetFarPlane(100.0);

        //Display
        isGridVisible = true;
        isAxisVisible = true;
        isGeoVisible = true;
        isGizmoVisible = true;
    }


    ImGui::End();
}

