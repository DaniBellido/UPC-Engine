#include "Globals.h"
#include "RingBufferModule.h"

#include "D3D12Module.h"
#include "Application.h"

// ------------------------------------------------------------
// Total size of the ring buffer (10 MB).
// This buffer will be reused every frame to store dynamic data
// such as constant buffers (PerFrame / PerInstance).
// ------------------------------------------------------------
#define MEMORY_TOTAL_SIZE 10 * (1 << 20)

RingBufferModule::RingBufferModule()
{
}

RingBufferModule::~RingBufferModule()
{
}

bool RingBufferModule::init()
{
    // ------------------------------------------------------------
    // Create a fixed-size buffer in the UPLOAD heap.
    //
    // Conceptually:
    // - This buffer lives in CPU-visible memory.
    // - We will write CPU data every frame.
    // - The GPU will read from it while rendering.
    //
    // The buffer behaves like a circular (ring) allocator.
    // ------------------------------------------------------------

    // ------------------------------------------------------------
    // The total size must be aligned to the constant buffer
    // alignment required by D3D12 (256 bytes).
    //
    // This guarantees that every allocation inside the buffer
    // can be safely used as a Constant Buffer View.
    // ------------------------------------------------------------

    totalMemorySize = alignUp(MEMORY_TOTAL_SIZE, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

    D3D12Module* d3d12 = app->getD3D12();
    ID3D12Device2* device = d3d12->getDevice();

    // ------------------------------------------------------------
    // Create an UPLOAD heap resource.
    //
    // Upload heap:
    // - CPU writes directly to it
    // - GPU can read from it
    // - Slower than DEFAULT heap, but ideal for dynamic data
    // ------------------------------------------------------------
    CD3DX12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(totalMemorySize);
    device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&buffer));
    buffer->SetName(L"Dynamic Ring Buffer");

    // ------------------------------------------------------------
   // Map the resource once and keep it mapped.
   //
   // Conceptually:
   // - bufferData is a raw CPU pointer to the entire ring buffer.
   // - We will manually manage offsets inside this memory.
   // ------------------------------------------------------------
    CD3DX12_RANGE readRange(0, 0); // We never read from CPU
    buffer->Map(0, &readRange, reinterpret_cast<void**>(&bufferData));

    // ------------------------------------------------------------
    // Initialize ring buffer state.
    //
    // head -> where the next allocation will happen
    // tail -> where old memory will be reclaimed
    // ------------------------------------------------------------
    head = 0;
    tail = 0;

    // Current frame index (used to know what memory can be freed)
    currentFrame = d3d12->getCurrentBackBufferIdx();
    totalAllocated = 0;

    // ------------------------------------------------------------
    // Track how much memory each frame has allocated.
    //
    // This allows us to safely reclaim memory once a frame
    // is finished on the GPU.
    // ------------------------------------------------------------
    for (int i = 0; i < FRAMES_IN_FLIGHT; ++i)
    {
        allocatedInFrame[i] = 0;
    }

    return true;
}

void RingBufferModule::preRender()
{
    // ------------------------------------------------------------
    // This function is called once per frame before rendering.
    //
    // Its responsibility is to reclaim memory that was used
    // by a previous frame and is now safe to reuse.
    // ------------------------------------------------------------
    D3D12Module* d3d12 = app->getD3D12();
    currentFrame = d3d12->getCurrentBackBufferIdx();

    // ------------------------------------------------------------
    // Move the tail forward by the amount of memory that
    // the current frame used last time it ran.
    //
    // Conceptually:
    // - The GPU has finished using this memory.
    // - We can safely mark it as free.
    // ------------------------------------------------------------
    tail = (tail + allocatedInFrame[currentFrame]) % totalMemorySize;
    totalAllocated -= allocatedInFrame[currentFrame]; // Update total allocated memory
    allocatedInFrame[currentFrame] = 0;  // Reset allocation counter for this frame
}

D3D12_GPU_VIRTUAL_ADDRESS RingBufferModule::allocBuffer(size_t size, void** cpuPtr)
{
    // ------------------------------------------------------------
    // Allocate a chunk of memory from the ring buffer.
    //
    // Conceptually:
    // - This is a linear allocator with wrap-around.
    // - head always points to the start of free memory.
    // - tail marks the boundary of memory still in use by the GPU.
    // ------------------------------------------------------------

    // ------------------------------------------------------------
    // 1. Align requested size to 256 bytes.
    //
    // D3D12 requires constant buffers to be aligned to
    // D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT.
    // ------------------------------------------------------------
    size = alignUp(size, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

    size_t freeSpace = 0;

    if (head >= tail)
    {
        // --------------------------------------------------------
        // Case A:
        // Free space is from head to the end of the buffer.
        //
        // [tail .... head ........ end]
        // --------------------------------------------------------
        freeSpace = totalMemorySize - head;

        if (size > freeSpace)
        {
            // ----------------------------------------------------
            // Not enough space at the end.
            // Try to wrap around to the beginning of the buffer.
            // ----------------------------------------------------
            if (size > tail)
            {
                // ------------------------------------------------
                // Not enough space even after wrapping.
                // Ring buffer is out of memory.
                // ------------------------------------------------
                Logger::Err("RingBuffer->NO MEMORY LEFT: needs " + std::to_string(size / 1024) +
                    "KB but only " + std::to_string(tail / 1024) + "KB total left");

                return 0;
            }
            // ----------------------------------------------------
            // Discard remaining space and wrap head to start.
            // ----------------------------------------------------
            head = 0;
        }
    }
    else
    {
        // --------------------------------------------------------
       // Case B:
       // Free space is between head and tail.
       //
       // [head .... free .... tail]
       // --------------------------------------------------------
        freeSpace = tail - head;

        if (size > freeSpace)
        {
            // ----------------------------------------------------
            // Not enough contiguous space.
            // Allocation fails.
            // ----------------------------------------------------
            Logger::Err("RingBuffer->MEMORY SPLIT: needs " + std::to_string(size / 1024) +
                "KB together but only " + std::to_string(freeSpace / 1024) + "KB in one piece");

            return 0;
        }
    }

    // ------------------------------------------------------------
    // 3. Compute GPU virtual address for this allocation.
    //
    // This is what will be bound to the pipeline.
    // ------------------------------------------------------------
    D3D12_GPU_VIRTUAL_ADDRESS gpuAddress = buffer->GetGPUVirtualAddress() + head;

    // ------------------------------------------------------------
     // 4. Return CPU pointer to the allocated memory.
     //
     // The caller will write constant buffer data here.
     // ------------------------------------------------------------
    if (cpuPtr)
    {
        *cpuPtr = bufferData + head;
    }

    // ------------------------------------------------------------
   // 5. Advance the head by the allocated size.
   //
   // Modulo makes the buffer circular.
   // ------------------------------------------------------------
    head += size;
    head %= totalMemorySize;

    // ------------------------------------------------------------
   // 6. Track how much memory this frame has used.
   //
   // This is critical for safe reclamation in preRender().
   // ------------------------------------------------------------
    allocatedInFrame[currentFrame] += size;
    totalAllocated += size;

    return gpuAddress;
}
