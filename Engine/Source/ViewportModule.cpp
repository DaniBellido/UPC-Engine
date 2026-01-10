#include "Globals.h"
#include "ViewportModule.h"

#include "D3D12Module.h"
#include "Application.h"
#include "ImGuiPass.h"

#include "EditorModule.h"
#include "ShaderDescriptorsModule.h"


ViewportModule::ViewportModule(HWND hWnd, D3D12Module* d3d12, ImGuiPass* imGuiPass)
{
    this->hWnd = hWnd;
    this->d3d12 = d3d12;
    this->imGuiPass = imGuiPass;
}

bool ViewportModule::init()
{
    Logger::Log("Initializing ViewportModule...");
    Timer t;
    t.Start();

    // ------------------------------------------------------------
    // Device & descriptor sizes
    // ------------------------------------------------------------
    ID3D12Device* device = d3d12->getDevice();
    rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    dsvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

    // ------------------------------------------------------------
    // Allocate a persistent SRV slot from the global shader-visible heap
    // ------------------------------------------------------------
    // The viewport color texture will be exposed to ImGui via this SRV handle.
    srvAllocator = app->getShaderDescriptors();
    srvIndex = srvAllocator->allocate();
    srvCpuHandle = srvAllocator->getCPUHandle(srvIndex);
    srvGpuHandle = srvAllocator->getGPUHandle(srvIndex);

    // ------------------------------------------------------------
    // Create RTV heap (1 descriptor) for the viewport render target
    // ------------------------------------------------------------
    D3D12_DESCRIPTOR_HEAP_DESC rtvDesc = {};
    rtvDesc.NumDescriptors = 1;
    rtvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    device->CreateDescriptorHeap(&rtvDesc, IID_PPV_ARGS(&rtvHeap));

    // ------------------------------------------------------------
    // Create DSV heap (1 descriptor) for the viewport depth buffer
    // ------------------------------------------------------------
    D3D12_DESCRIPTOR_HEAP_DESC dsvDesc = {};
    dsvDesc.NumDescriptors = 1;
    dsvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    device->CreateDescriptorHeap(&dsvDesc, IID_PPV_ARGS(&dsvHeap));

    // ------------------------------------------------------------
   // Create initial GPU resources (color RT + depth)
   // ------------------------------------------------------------
   // We create a default-size target so the viewport has a valid SRV from frame 1.
   // Resources are released in cleanUp() to prevent DX "Live Objects" warnings.
    createResources(width, height); //commented to avoid breaks when closing the app (DX Live Objects)

    t.Stop();
    Logger::Log("ViewportModule initialized in: " + std::to_string(t.ReadMs()) + " ms.");

	return true;
}

void ViewportModule::preRender()
{
    if (!visible)
        return;

    // ------------------------------------------------------------
    // Viewport window
    // ------------------------------------------------------------
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

    ImGuiIO& io = ImGui::GetIO();
    bool old = io.ConfigWindowsMoveFromTitleBarOnly;
    io.ConfigWindowsMoveFromTitleBarOnly = true;

    ImGui::Begin("Viewport", &visible, flags);
    drawList = ImGui::GetWindowDrawList();
    viewportPos = ImGui::GetCursorScreenPos();
    viewportSize = ImGui::GetContentRegionAvail();

    // ------------------------------------------------------------
    // Clamp content area (avoid 0x0 targets)
    // ------------------------------------------------------------
    viewportSize.x = std::max(1.0f, viewportSize.x);
    viewportSize.y = std::max(1.0f, viewportSize.y);

    uint32_t newW = (uint32_t)viewportSize.x;
    uint32_t newH = (uint32_t)viewportSize.y;

    // ------------------------------------------------------------
    // Resize handling (recreate RT/DS when the ImGui region changes)
    // ------------------------------------------------------------
    if (newW != width || newH != height)
        pendingResize = true;

    handleResize();

    // ------------------------------------------------------------
    // Draw viewport image (SRV) - this is what the user sees
    // ------------------------------------------------------------
    ImGui::Image((ImTextureID)srvGpuHandle.ptr, viewportSize);

    // ------------------------------------------------------------
    // Window interaction state (used by camera controls / gizmos)
    // ------------------------------------------------------------
    hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
    focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

    ImGui::End();
}

bool ViewportModule::cleanUp()
{
    // ------------------------------------------------------------
    // GPU resources
    // ------------------------------------------------------------
    destroyResources();  

    // ------------------------------------------------------------
    // Descriptor heaps
    // ------------------------------------------------------------
    if (rtvHeap) rtvHeap.Reset();
    if (dsvHeap) dsvHeap.Reset();

    imGuiPass = nullptr;

	return true;
}

void ViewportModule::transitionToRenderTarget(ID3D12GraphicsCommandList* cmd)
{
    // ------------------------------------------------------------
    // Viewport texture: SRV -> RTV
    // ------------------------------------------------------------
    // Before the scene renders into the viewport texture, we must transition it
    // from PIXEL_SHADER_RESOURCE (ImGui sampling) to RENDER_TARGET.
    if (!colorTexture) return;

    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(colorTexture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
    cmd->ResourceBarrier(1, &barrier);
}

void ViewportModule::transitionToShaderResource(ID3D12GraphicsCommandList* cmd)
{
    // ------------------------------------------------------------
    // Viewport texture: RTV -> SRV
    // ------------------------------------------------------------
    // After scene rendering, we transition back to PIXEL_SHADER_RESOURCE so ImGui
    // can sample it in ImGui::Image.
    if (!colorTexture) return;

    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(colorTexture, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    cmd->ResourceBarrier(1, &barrier);
}

bool ViewportModule::isUsable() const
{
    return visible && width > 0 && height > 0 && colorTexture != nullptr;
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
    clearColor.Color[0] = 0.2f;
    clearColor.Color[1] = 0.2f;
    clearColor.Color[2] = 0.2f;
    clearColor.Color[3] = 1.0f;

    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);

    device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &colorDesc, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, &clearColor, IID_PPV_ARGS(&colorTexture));

    // RTV (CPU-only heap)
    rtvHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
    device->CreateRenderTargetView(colorTexture, nullptr, rtvHandle);

    // SRV (global shader-visible heap)
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

    device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &depthDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &clearDepth, IID_PPV_ARGS(&depthTexture));

    // DSV (CPU-only heap)
    dsvHandle = dsvHeap->GetCPUDescriptorHandleForHeapStart();
    device->CreateDepthStencilView(depthTexture, nullptr, dsvHandle);
}

void ViewportModule::destroyResources()
{
    // ------------------------------------------------------------
    // Release GPU resources (RT + DS)
    // ------------------------------------------------------------
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

    // Reset descriptor handles (heaps remain alive)
    rtvHandle = {};
    dsvHandle = {};

}

void ViewportModule::handleResize()
{
    if (!pendingResize)
        return;

    // ------------------------------------------------------------
    // Resize requires GPU/CPU synchronization
    // ------------------------------------------------------------
    // We must ensure the GPU is not using the old textures before releasing them.
    d3d12->waitForGPU();

    destroyResources();
    createResources((uint32_t)std::max(1.0f, viewportSize.x),(uint32_t)std::max(1.0f, viewportSize.y));

    pendingResize = false;
}
