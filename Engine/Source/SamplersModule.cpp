#include "Globals.h"
#include "SamplersModule.h"
#include "Application.h"
#include "D3D12Module.h"

bool SamplersModule::init()
{
    ID3D12Device* device = app->getD3D12()->getDevice();

    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
    desc.NumDescriptors = MAX_SAMPLERS;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    HRESULT hr = device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&samplerHeap));
    if (FAILED(hr)) return false;

    descriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
    nextFreeSlot = 0;
    return true;
}

UINT SamplersModule::createSampler(const D3D12_SAMPLER_DESC& desc)
{
    if (nextFreeSlot >= MAX_SAMPLERS) return UINT_MAX;

    ID3D12Device* device = app->getD3D12()->getDevice();
    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = getCPUHandle(nextFreeSlot);

    device->CreateSampler(&desc, cpuHandle);
    return nextFreeSlot++;
}

D3D12_CPU_DESCRIPTOR_HANDLE SamplersModule::getCPUHandle(UINT index) const
{
    D3D12_CPU_DESCRIPTOR_HANDLE handle = samplerHeap->GetCPUDescriptorHandleForHeapStart();
    handle.ptr += index * descriptorSize;
    return handle;
}

D3D12_GPU_DESCRIPTOR_HANDLE SamplersModule::getGPUHandle(UINT index) const
{
    D3D12_GPU_DESCRIPTOR_HANDLE handle = samplerHeap->GetGPUDescriptorHandleForHeapStart();
    handle.ptr += index * descriptorSize;
    return handle;
}
