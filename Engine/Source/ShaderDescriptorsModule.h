#pragma once
#include "Module.h"

	/*class that will manage :
	A single Shader - Visible Descriptor Heap for CBV, SRV, and UAV descriptors.
	An index for tracking the next free slot in the heap.
	Utility functions for descriptor allocation and heap management.*/

class ShaderDescriptorsModule : public Module
{
private:
    ComPtr<ID3D12DescriptorHeap> descriptorHeap;
    UINT nextFreeSlot = 0;
    UINT descriptorSize = 0;
    static const UINT MAX_DESCRIPTORS = 1024;

public:
    ShaderDescriptorsModule() {}
    ~ShaderDescriptorsModule() {}

    bool init() override;
    void reset();

    UINT allocate();
    UINT createSRV(ID3D12Resource* resource);

    D3D12_CPU_DESCRIPTOR_HANDLE getCPUHandle(UINT index) const;
    D3D12_GPU_DESCRIPTOR_HANDLE getGPUHandle(UINT index) const;

    ID3D12DescriptorHeap* getHeap() const { return descriptorHeap.Get(); }


};

