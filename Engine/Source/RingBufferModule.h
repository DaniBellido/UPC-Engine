#pragma once
#include "Module.h"

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

	D3D12_GPU_VIRTUAL_ADDRESS allocBuffer();
};

