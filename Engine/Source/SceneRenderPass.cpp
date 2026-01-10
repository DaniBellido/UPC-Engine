#include "Globals.h"
#include "SceneRenderPass.h"

#include "Application.h"
#include "D3D12Module.h"
#include "ViewportModule.h"

void SceneRenderPass::begin(ID3D12GraphicsCommandList* cmd, const float clearColor[4]) const
{
    // ------------------------------------------------------------
    // Resource state (Viewport only)
    // ------------------------------------------------------------
    // If rendering to the editor viewport texture, transition it from SRV
    // (ImGui sampling) to RTV (scene rendering).
    if (usingViewport && viewport)
        viewport->transitionToRenderTarget(cmd);

    // ------------------------------------------------------------
    // Raster state: viewport & scissor
    // ------------------------------------------------------------
    // Match rasterization to the selected render target size.
    D3D12_VIEWPORT vp{ 0.0f, 0.0f, float(width), float(height), 0.0f, 1.0f };
    D3D12_RECT sc{ 0, 0, (LONG)width, (LONG)height };
    cmd->RSSetViewports(1, &vp);
    cmd->RSSetScissorRects(1, &sc);

    // ------------------------------------------------------------
    // Output merger: bind targets + clear
    // ------------------------------------------------------------
    cmd->OMSetRenderTargets(1, &rtv, FALSE, &dsv);  // Bind color + depth targets
    if(clearColor)
        cmd->ClearRenderTargetView(rtv, clearColor, 0, nullptr); // Clear color buffer
    cmd->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr); // Clear depth (and stencil if used)

}

void SceneRenderPass::end(ID3D12GraphicsCommandList* cmd) const
{
    // ------------------------------------------------------------
    // Resource state (Viewport only)
    // ------------------------------------------------------------
    // Restore the viewport texture to SRV so ImGui can sample it in ImGui::Image
    if (usingViewport && viewport)
        viewport->transitionToShaderResource(cmd);
}

SceneRenderPass GetSceneRenderPass(Application* app)
{
    SceneRenderPass pass{};

    // ------------------------------------------------------------
    // Acquire modules
    // ------------------------------------------------------------
    auto* d3d12 = app->getD3D12();
    auto* vp = app->getViewport();

    // ------------------------------------------------------------
    // Target selection
    // ------------------------------------------------------------
    // Prefer the viewport texture when it's visible and has a valid size.
    pass.usingViewport = (vp && vp->isUsable());

    if (pass.usingViewport)
    {
        // Viewport path (offscreen texture)
        pass.viewport = vp;
        pass.rtv = vp->getRTV();
        pass.dsv = vp->getDSV();
        pass.width = vp->getWidth();
        pass.height = vp->getHeight();
    }
    else
    {
        // Backbuffer path (swapchain)
        pass.viewport = nullptr;
        pass.rtv = d3d12->getRenderTargetDescriptor();
        pass.dsv = d3d12->getDepthStencilDescriptor();
        pass.width = d3d12->getWindowWidth();
        pass.height = d3d12->getWindowHeight();
    }

    // ------------------------------------------------------------
    // Derived values
    // ------------------------------------------------------------
    // Aspect ratio is used by exercises to build a correct projection matrix.
    pass.aspect = (pass.height > 0) ? (float(pass.width) / float(pass.height)) : 1.0f;

    return pass;
}
