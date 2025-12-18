#pragma once
#include "Module.h"

class D3D12Module;
class ShaderDescriptorsModule;
class ImGuiPass;

class ViewportModule : public Module
{
private:
    // --------------------------------------------------
    // Platform / Core
    // --------------------------------------------------
    HWND hWnd = NULL;
    D3D12Module* d3d12 = nullptr;
    ImGuiPass* imGuiPass = nullptr;
    ShaderDescriptorsModule* srvAllocator = nullptr;

    // --------------------------------------------------
    // State
    // --------------------------------------------------
    bool visible = false;
    bool focused = false;
    bool hovered = false;

    // --------------------------------------------------
    // Viewport size (ImGui driven)
    // --------------------------------------------------
    uint32_t width = 1280;
    uint32_t height = 720;

    ImVec2 viewportSize = {};
    ImVec2 viewportPos = {};

    bool pendingResize = false;

    // --------------------------------------------------
    // GPU resources
    // --------------------------------------------------
    ID3D12Resource* colorTexture = nullptr; // Render target
    ID3D12Resource* depthTexture = nullptr; // Depth buffer

    // --------------------------------------------------
    // Descriptors
    // --------------------------------------------------
    // RTV (rendering)
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = {};

    // DSV (depth)
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = {};

    // SRV (ImGui / shaders)
    D3D12_CPU_DESCRIPTOR_HANDLE srvCpuHandle = {};
    D3D12_GPU_DESCRIPTOR_HANDLE srvGpuHandle = {};

    // --------------------------------------------------
    // Heaps
    // --------------------------------------------------
    ComPtr<ID3D12DescriptorHeap> rtvHeap;
    ComPtr<ID3D12DescriptorHeap> dsvHeap;

    UINT rtvDescriptorSize = 0;
    UINT dsvDescriptorSize = 0;

    // --------------------------------------------------
    // Internal helpers
    // --------------------------------------------------
    void createResources(uint32_t width, uint32_t height);
    void destroyResources();
    void handleResize();

public:
    ViewportModule(HWND hWnd, D3D12Module* d3d12, ImGuiPass* imGuiPass);
    ~ViewportModule() = default;

    bool init() override;
    void preRender() override;
    void render() override;
    bool cleanUp() override;

    bool isVisible() const { return visible; }
    void setVisible(bool value) { visible = value; }

    bool isFocused() const { return focused; }
    bool isHovered() const { return hovered; }

    D3D12_CPU_DESCRIPTOR_HANDLE getRTV() const { return rtvHandle; }
    D3D12_CPU_DESCRIPTOR_HANDLE getDSV() const { return dsvHandle; }

    D3D12_GPU_DESCRIPTOR_HANDLE getSRV() const { return srvGpuHandle; }

    uint32_t getWidth() const { return width; }
    uint32_t getHeight() const { return height; }
};

