#pragma once
#include "Module.h"
#include "D3D12Module.h"
#include "DirectXTex.h"
#include <filesystem>

// ------------------------------------------------------------------------------------------
// ResourcesModule handles creation and management of GPU resources in DirectX 12.
// It provides functions to create buffers, textures, render targets, and depth stencils.
// The class manages temporary upload buffers and command lists for resource initialization.
// ------------------------------------------------------------------------------------------

class ResourcesModule : public Module
{
private:

	ComPtr<ID3D12CommandAllocator> commandAllocator;
	ComPtr<ID3D12GraphicsCommandList> commandList;

public:
	ResourcesModule();
	~ResourcesModule();

	bool init() override;
	//void preRender() override;
	bool cleanUp() override;
	
	ComPtr<ID3D12Resource> createUploadBuffer(const void* data, size_t size, const char* name);
	ComPtr<ID3D12Resource> createDefaultBuffer(const void* data, size_t size, const char* name);

	ComPtr<ID3D12Resource> createRawTexture2D(const void* data, size_t rowSize, size_t width, size_t height, DXGI_FORMAT format);
	ComPtr<ID3D12Resource> createTextureFromMemory(const void* data, size_t size, const char* name);
	ComPtr<ID3D12Resource> createTextureFromFile(const std::filesystem::path& path, bool defaultSRGB = false);

private:

	ComPtr<ID3D12Resource> createTextureFromImage(const ScratchImage& image, const char* name);
};

