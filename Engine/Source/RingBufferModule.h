#pragma once
#include "Module.h"

// ----------------------------------------------------------------------------
// RingBufferModule
// ----------------------------------------------------------------------------
// This module implements a dynamic ring (circular) buffer allocator on top of
// a D3D12 UPLOAD heap resource.
//
// Purpose:
// - Provide fast, transient allocations for dynamic GPU data such as
//   constant buffers (PerFrame / PerInstance).
// - Avoid creating and destroying GPU resources every frame.
// - Allow CPU writes and GPU reads without stalls by reusing memory safely
//   across multiple frames in flight.
//
// How it works (conceptual overview):
// - A single large upload buffer is created and kept persistently mapped.
// - Memory inside the buffer is managed manually using a ring allocator:
//     * 'head' marks where the next allocation will occur.
//     * 'tail' marks the oldest memory still in use by the GPU.
// - Allocations advance the head.
// - At the beginning of each frame, memory used by completed frames is
//   reclaimed by advancing the tail.
// - Memory is reused in a circular fashion once the end of the buffer is reached.
//
// Frame safety:
// - The module tracks how much memory each frame allocates.
// - This ensures memory is never overwritten while still in use by the GPU,
//   even with multiple frames in flight.
//
// Typical usage:
// - Call allocBuffer() during rendering to upload constant buffer data.
// - Bind the returned GPU virtual address to the pipeline.
// - The CPU pointer returned allows writing the data directly.
//
// This class is infrastructure-only:
// - It does NOT know what data is stored (PerFrame, PerInstance, etc.).
// - It only manages when and where memory is allocated and reclaimed.
// ============================================================================

class RingBufferModule : public Module
{
private:
    char* bufferData = nullptr;
    ComPtr<ID3D12Resource> buffer;
    size_t                 totalMemorySize = 0;
    size_t                 head = 0;
    size_t                 tail = 0;
    size_t                 allocatedInFrame[FRAMES_IN_FLIGHT];
    size_t                 totalAllocated = 0;
    unsigned               currentFrame = 0;

public:
	RingBufferModule();
	~RingBufferModule();

	bool init() override;
	void preRender() override;

	D3D12_GPU_VIRTUAL_ADDRESS allocBuffer(size_t size, void** cpuPtr);

    size_t getTotalSize() const { return totalMemorySize; }
    size_t getHead() const { return head; }
    size_t getTail() const { return tail; }
    size_t getTotalAllocated() const { return totalAllocated; }
    size_t getAllocatedInFrame(int i) const { return allocatedInFrame[i]; }
    unsigned getCurrentFrame() const { return currentFrame; }
};

