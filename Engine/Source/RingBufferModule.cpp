#include "Globals.h"
#include "RingBufferModule.h"

#include "D3D12Module.h"
#include "Application.h"

// 10 Mb
#define MEMORY_TOTAL_SIZE 10 * (1 << 20)

RingBufferModule::RingBufferModule()
{
}

RingBufferModule::~RingBufferModule()
{
}

bool RingBufferModule::init()
{
    //Creates fixed size buffer in the Upload Heap

    //Size should be multiple of Connstant Buffer Data Placement Alignment

    // Map the resource using ID3D12Resource::Map method to get CPU pointer

    totalMemorySize = alignUp(MEMORY_TOTAL_SIZE, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

    D3D12Module* d3d12 = app->getD3D12();
    ID3D12Device2* device = d3d12->getDevice();
    ID3D12CommandQueue* queue = d3d12->getCommandQueue();

    CD3DX12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(totalMemorySize);
    device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&buffer));
    buffer->SetName(L"Dynamic Ring Buffer");

    CD3DX12_RANGE readRange(0, 0);
    buffer->Map(0, &readRange, reinterpret_cast<void**>(&bufferData));

    head = 0;
    tail = 0;
    currentFrame = d3d12->getCurrentBackBufferIdx();
    totalAllocated = 0;

    for (int i = 0; i < FRAMES_IN_FLIGHT; ++i)
    {
        allocatedInFrame[i] = 0;
    }

    return true;
}

void RingBufferModule::preRender()
{
    // Function used to reclaim previous finished frames memory
    D3D12Module* d3d12 = app->getD3D12();

    currentFrame = d3d12->getCurrentBackBufferIdx();

    tail = (tail + allocatedInFrame[currentFrame]) % totalMemorySize;
    totalAllocated -= allocatedInFrame[currentFrame];

    allocatedInFrame[currentFrame] = 0;
}

D3D12_GPU_VIRTUAL_ADDRESS RingBufferModule::allocBuffer()
{
    // Track three key values:
    // Head -> Start of free memory
    // Tail -> End of free memory
    // Total buffer size

   // Return GPU_VIRTUAL_ADDRESS = GetGPUVirtualAddress + Head.

    return D3D12_GPU_VIRTUAL_ADDRESS();
}
