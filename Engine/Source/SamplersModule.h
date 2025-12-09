#pragma once
#include "Module.h"
class SamplersModule : public Module
{
private:
    ComPtr<ID3D12DescriptorHeap> samplerHeap;
    UINT descriptorSize = 0;
    UINT nextFreeSlot = 0;
    static const UINT MAX_SAMPLERS = 64;


public:
    SamplersModule() {}
    ~SamplersModule() {}

    bool init() override;

    UINT createSampler(const D3D12_SAMPLER_DESC& desc);

    D3D12_CPU_DESCRIPTOR_HANDLE getCPUHandle(UINT index) const;
    D3D12_GPU_DESCRIPTOR_HANDLE getGPUHandle(UINT index) const;

    ID3D12DescriptorHeap* getHeap() const { return samplerHeap.Get(); }
    UINT getDescriptorSize() const { return descriptorSize; }
};

