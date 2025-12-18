#include "Globals.h"
#include "ShaderDescriptorsModule.h"
#include "Application.h"
#include "D3D12Module.h"

bool ShaderDescriptorsModule::init()
{
    D3D12Module* d3d12 = app->getD3D12();
    ID3D12Device2* device = d3d12->getDevice();

    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.NumDescriptors = MAX_DESCRIPTORS;
    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    if (FAILED(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&descriptorHeap))))
        return false;

    descriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    nextFreeSlot = 0;

    return true;
}

void ShaderDescriptorsModule::reset()
{
    nextFreeSlot = 0;
}

UINT ShaderDescriptorsModule::allocate()
{
    return nextFreeSlot++;
}

UINT ShaderDescriptorsModule::createSRV(ID3D12Resource* resource)
{

    if (!resource)
    {
        OutputDebugStringA("Error: createSRV called with nullptr resource!\n");
        return UINT_MAX; 
    }

    UINT index = allocate();

    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = getCPUHandle(index);

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = resource->GetDesc().Format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2D.MipLevels = resource->GetDesc().MipLevels;

    app->getD3D12()->getDevice()->CreateShaderResourceView(resource, &srvDesc, cpuHandle);

    return index;
}

D3D12_CPU_DESCRIPTOR_HANDLE ShaderDescriptorsModule::getCPUHandle(UINT index) const
{
    D3D12_CPU_DESCRIPTOR_HANDLE handle = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
    handle.ptr += index * descriptorSize;
    return handle;
}

D3D12_GPU_DESCRIPTOR_HANDLE ShaderDescriptorsModule::getGPUHandle(UINT index) const
{
    D3D12_GPU_DESCRIPTOR_HANDLE handle = descriptorHeap->GetGPUDescriptorHandleForHeapStart();
    handle.ptr += index * descriptorSize;
    return handle;
}


