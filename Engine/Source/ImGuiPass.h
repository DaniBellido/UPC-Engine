#pragma once


class ImGuiPass
{
    ComPtr<ID3D12DescriptorHeap> heap;

public:
    ImGuiPass(ID3D12Device2* device, HWND hWnd, D3D12_CPU_DESCRIPTOR_HANDLE cpuTextHandle = { 0 }, D3D12_GPU_DESCRIPTOR_HANDLE gpuTextHandle = { 0 });
    ~ImGuiPass();

    void startFrame();
    void record(ID3D12GraphicsCommandList* commandList, D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle);

    D3D12_CPU_DESCRIPTOR_HANDLE getCpuHandle() const { return heap ? heap->GetCPUDescriptorHandleForHeapStart() : D3D12_CPU_DESCRIPTOR_HANDLE{}; }
    D3D12_GPU_DESCRIPTOR_HANDLE getGpuHandle() const { return heap ? heap->GetGPUDescriptorHandleForHeapStart() : D3D12_GPU_DESCRIPTOR_HANDLE{}; }
};
