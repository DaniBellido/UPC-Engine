#pragma once
#include "Module.h"
#include "D3D12Module.h"

class ResourcesModule : public Module
{
private:
	D3D12Module* d3d12 = nullptr;

	ComPtr<ID3D12CommandAllocator> commandAllocator;
	ComPtr<ID3D12GraphicsCommandList> commandList;

public:
	bool init() override;
	void preRender() override;
	bool cleanUp() override;
	
	ComPtr<ID3D12Resource> createUploadBuffer(const void* data, size_t size, const char* name);
	ComPtr<ID3D12Resource> createDefaultBuffer(const void* data, size_t size, const char* name);

};

