#pragma once

#include <windows.h>
#include <wrl.h>
#include <d3d12.h>

class DDRenderInterfaceCoreD3D12;

// DebugDrawPass provides an interface for rendering debug geometry (lines, points, text, etc.) in a DirectX 12 application.
// It wraps the DebugDraw library's D3D12 implementation, manages its lifetime, and exposes a simple API for recording debug draw commands.
// Use this class to visualize geometry and diagnostics during development and debugging of graphics applications.
class DebugDrawPass 
{

public:

    DebugDrawPass(ID3D12Device4* device, ID3D12CommandQueue* uploadQueue, D3D12_CPU_DESCRIPTOR_HANDLE cpuText = { 0 }, D3D12_GPU_DESCRIPTOR_HANDLE gpuText = { 0 });

    ~DebugDrawPass();

    void record(ID3D12GraphicsCommandList* commandList, uint32_t width, uint32_t height, const Matrix& view ,const Matrix& proj);

private:

    static DDRenderInterfaceCoreD3D12* implementation;
};
