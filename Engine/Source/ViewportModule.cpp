#include "Globals.h"
#include "ViewportModule.h"
#include "ShaderDescriptorsModule.h"
#include "D3D12Module.h"
#include "Application.h"



ViewportModule::ViewportModule(HWND hWnd, D3D12Module* d3d12)
{
    this->hWnd = hWnd;
    this->d3d12 = d3d12;
}

bool ViewportModule::init()
{
    Logger::Log("Initializing ViewportModule...");
    Timer t;
    t.Start();

    ID3D12Device* device = d3d12->getDevice();

    rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    dsvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

    srvAllocator = app->getShaderDescriptors();

    // RTV heap (1 descriptor)
    D3D12_DESCRIPTOR_HEAP_DESC rtvDesc = {};
    rtvDesc.NumDescriptors = 1;
    rtvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    device->CreateDescriptorHeap(&rtvDesc, IID_PPV_ARGS(&rtvHeap));

    // DSV heap (1 descriptor)
    D3D12_DESCRIPTOR_HEAP_DESC dsvDesc = {};
    dsvDesc.NumDescriptors = 1;
    dsvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    device->CreateDescriptorHeap(&dsvDesc, IID_PPV_ARGS(&dsvHeap));

    // createResources(width, height); commented to avoid breaks when closing the app (DX Live Objects)

    t.Stop();
    Logger::Log("ViewportModule initialized in: " + std::to_string(t.ReadMs()) + " ms.");

	return true;
}

void ViewportModule::preRender()
{
    if (!visible)
        return;

    // Viewport window
    ImGui::Begin("Viewport", &visible);

    viewportPos = ImGui::GetCursorScreenPos();
    viewportSize = ImGui::GetContentRegionAvail();

    if ((uint32_t)viewportSize.x != width || (uint32_t)viewportSize.y != height)
    {
        pendingResize = true;
    }

    /*ImGui::Image((ImTextureID)srvGpuHandle.ptr, viewportSize);*/


    ImGui::End();
}

void ViewportModule::render()
{
}

bool ViewportModule::cleanUp()
{
	return true;
}


void ViewportModule::createResources(uint32_t w, uint32_t h)
{
    ID3D12Device* device = d3d12->getDevice();

    width = w;
    height = h;

    // --------------------------------------------------
    // Color texture (Render Target)
    // --------------------------------------------------
    D3D12_RESOURCE_DESC colorDesc = {};
    colorDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    colorDesc.Width = width;
    colorDesc.Height = height;
    colorDesc.DepthOrArraySize = 1;
    colorDesc.MipLevels = 1;
    colorDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    colorDesc.SampleDesc.Count = 1;
    colorDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

    D3D12_CLEAR_VALUE clearColor = {};
    clearColor.Format = colorDesc.Format;
    clearColor.Color[3] = 1.0f;

    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);

    device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &colorDesc,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        &clearColor,
        IID_PPV_ARGS(&colorTexture)
    );

    // RTV
    rtvHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
    device->CreateRenderTargetView(colorTexture, nullptr, rtvHandle);

    // --------------------------------------------------
    // SRV (for ImGui)
    // --------------------------------------------------
    UINT srvIndex = srvAllocator->createSRV(colorTexture);
    srvCpuHandle = srvAllocator->getCPUHandle(srvIndex);
    srvGpuHandle = srvAllocator->getGPUHandle(srvIndex);

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = colorDesc.Format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    device->CreateShaderResourceView(colorTexture, &srvDesc, srvCpuHandle);

    // --------------------------------------------------
    // Depth texture
    // --------------------------------------------------
    D3D12_RESOURCE_DESC depthDesc = colorDesc;
    depthDesc.Format = DXGI_FORMAT_D32_FLOAT;
    depthDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE clearDepth = {};
    clearDepth.Format = depthDesc.Format;
    clearDepth.DepthStencil.Depth = 1.0f;

    device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &depthDesc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &clearDepth,
        IID_PPV_ARGS(&depthTexture)
    );

    dsvHandle = dsvHeap->GetCPUDescriptorHandleForHeapStart();
    device->CreateDepthStencilView(depthTexture, nullptr, dsvHandle);
}

void ViewportModule::destroyResources()
{
    if (colorTexture)
    {
        colorTexture->Release();
        colorTexture = nullptr;
    }

    if (depthTexture)
    {
        depthTexture->Release();
        depthTexture = nullptr;
    }

    rtvHandle = {};
    dsvHandle = {};
    srvCpuHandle = {};
    srvGpuHandle = {};
}

void ViewportModule::handleResize()
{
    if (!pendingResize)
        return;

    destroyResources();
    createResources((uint32_t)viewportSize.x, (uint32_t)viewportSize.y);

    pendingResize = false;
}
