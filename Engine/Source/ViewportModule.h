#pragma once

// ============================================================================
// ViewportModule
// ----------------------------------------------------------------------------
// This module owns the editor "Viewport" offscreen render target.
//
// Purpose:
// - Provide an offscreen color + depth target where exercises can render the
//   3D scene.
// - Expose the color target as an SRV so it can be displayed in ImGui using
//   ImGui::Image.
// - Track the ImGui viewport rectangle (position/size) for editor tools such
//   as ImGuizmo, mouse picking, and camera controls.
//
// Key responsibilities:
// - Create and destroy the viewport color/depth GPU resources.
// - Recreate resources when the ImGui viewport region is resized.
// - Manage the viewport texture state transitions:
//     * SRV  -> RTV before scene rendering
//     * RTV  -> SRV after scene rendering (ready for ImGui sampling)
// ============================================================================

#include "Module.h"

class D3D12Module;
class ShaderDescriptorsModule;
class ImGuiPass;

class ViewportModule : public Module
{
private:
    // --------------------------------------------------
    // Core
    // --------------------------------------------------
    HWND hWnd = NULL;
    D3D12Module* d3d12 = nullptr;
    ImGuiPass* imGuiPass = nullptr;
    ShaderDescriptorsModule* srvAllocator = nullptr;

    // --------------------------------------------------
    // ImGui State
    // --------------------------------------------------
    bool visible = true;
    bool focused = false;
    bool hovered = false;
    ImDrawList* drawList = nullptr;

    // --------------------------------------------------
    // Viewport size 
    // --------------------------------------------------
    uint32_t width = 1280;
    uint32_t height = 720;
    ImVec2 viewportSize = {};
    ImVec2 viewportPos = {};

    bool pendingResize = false;

    // --------------------------------------------------
    // GPU resources
    // --------------------------------------------------
    ID3D12Resource* colorTexture = nullptr; // Offscreen render target
    ID3D12Resource* depthTexture = nullptr; // Offscreen depth buffer

    // --------------------------------------------------
    // Descriptors
    // --------------------------------------------------
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = {}; // for colorTexture
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = {}; // for depthTexture

    // SRV handle for sampling the viewport texture (ImGui::Image)
    D3D12_CPU_DESCRIPTOR_HANDLE srvCpuHandle = {};
    D3D12_GPU_DESCRIPTOR_HANDLE srvGpuHandle = {};
    uint32_t srvIndex = 0;

    // Descriptor heaps (CPU-only)
    ComPtr<ID3D12DescriptorHeap> rtvHeap;
    ComPtr<ID3D12DescriptorHeap> dsvHeap;

    UINT rtvDescriptorSize = 0;
    UINT dsvDescriptorSize = 0;

    // --------------------------------------------------
    // Helpers
    // --------------------------------------------------
    void createResources(uint32_t width, uint32_t height);
    void destroyResources();
    void handleResize();

public:
    ViewportModule(HWND hWnd, D3D12Module* d3d12, ImGuiPass* imGuiPass);
    ~ViewportModule() = default;

    bool init() override;
    void preRender() override;
    bool cleanUp() override;

    // ------------------------------------------------------------
    // Visibility / interaction
    // ------------------------------------------------------------
    bool isVisible() const { return visible; }
    void setVisible(bool value) { visible = value; }

    bool isFocused() const { return focused; }
    bool isHovered() const { return hovered; }

    bool isUsable() const;

    // ------------------------------------------------------------
    // Render target access
    // ------------------------------------------------------------
    D3D12_CPU_DESCRIPTOR_HANDLE getRTV() const { return rtvHandle; }
    D3D12_CPU_DESCRIPTOR_HANDLE getDSV() const { return dsvHandle; }
    D3D12_GPU_DESCRIPTOR_HANDLE getSRV() const { return srvGpuHandle; }

    ID3D12Resource* getColorTexture() const { return colorTexture; }

    // ------------------------------------------------------------
    // ImGui viewport info (for gizmos / picking)
    // ------------------------------------------------------------
    ImDrawList* getDrawList() const { return drawList; }
    ImVec2 getViewportPos()  const { return viewportPos; }
    ImVec2 getViewportSize() const { return viewportSize; }

    uint32_t getWidth() const { return width; }
    uint32_t getHeight() const { return height; }

    // ------------------------------------------------------------
    // Resource transitions (viewport texture owner)
    // ------------------------------------------------------------
    void transitionToRenderTarget(ID3D12GraphicsCommandList* cmd);
    void transitionToShaderResource(ID3D12GraphicsCommandList* cmd);
};

