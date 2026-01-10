#pragma once

// ============================================================================
// SceneRenderPass
// ----------------------------------------------------------------------------
// A small "pass object" that encapsulates the render target setup required to
// render the 3D scene either to:
//
//   1) The editor Viewport render texture (offscreen), or
//   2) The swapchain backbuffer (fullscreen)
//
// Purpose:
// - Centralize the boilerplate required to begin/end a scene render pass.
// - Keep exercises clean by hiding target selection + viewport/scissor + clears.
// - Provide a reusable template for any exercise that needs to render a scene.
//
// What it contains:
// - Target descriptors (RTV/DSV).
// - Target dimensions (width/height) and derived aspect ratio.
// - Optional pointer to ViewportModule when rendering offscreen.
//
// Responsibilities:
// - begin(): prepares the render target for scene rendering.
//     * (Viewport path) transitions the viewport texture to RENDER_TARGET state.
//     * sets D3D12 viewport and scissor to match the chosen target.
//     * binds RTV/DSV and clears color + depth.
// - end(): restores the target state after scene rendering.
//     * (Viewport path) transitions the viewport texture back to SRV so ImGui
//       can sample it (ImGui::Image).
//
// Notes:
// - SceneRenderPass does NOT own the resources. It only references them.
// - The viewport transitions are delegated to ViewportModule, since it owns
//   the offscreen texture and knows the intended final state (SRV).
// ============================================================================

class Application;
class ViewportModule;
struct ID3D12GraphicsCommandList;

struct SceneRenderPass
{
    // True if we are rendering to the editor viewport texture (offscreen).
    bool usingViewport = false;

    // Render target descriptors used by OMSetRenderTargets.
    D3D12_CPU_DESCRIPTOR_HANDLE rtv{};
    D3D12_CPU_DESCRIPTOR_HANDLE dsv{};

    // Target size and derived aspect ratio for projection.
    uint32_t width = 0;
    uint32_t height = 0;
    float aspect = 1.0f;

    // Owner used for resource state transitions (viewport texture).
    ViewportModule* viewport = nullptr;

    void begin(ID3D12GraphicsCommandList* cmd, const float clearColor[4]) const;
    void end(ID3D12GraphicsCommandList* cmd) const;
};

// Builds a SceneRenderPass that targets either the Viewport texture (if usable)
// or the swapchain backbuffer otherwise.
SceneRenderPass GetSceneRenderPass(Application* app);