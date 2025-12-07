#include "Globals.h"
#include "ResourcesModule.h"
#include "Application.h"



ResourcesModule::ResourcesModule() 
{
}


ResourcesModule::~ResourcesModule() 
{
}

bool ResourcesModule::init()
{
	Logger::Log("Initializing ResourcesModule...");
	Timer t;
	t.Start();

	D3D12Module* d3d12 = app->getD3D12();
	ID3D12Device4* device = d3d12->getDevice();

	device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator));
	device->CreateCommandList1(0, D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(&commandList));
	commandList->Reset(commandAllocator.Get(), nullptr);
	commandList->Close();

	t.Stop();
	Logger::Log("ResourceModule initialized in: " + std::to_string(t.ReadMs()) + " ms.");

	return true;
}

bool ResourcesModule::cleanUp()
{
	return true;
}


// ----------------------------------------------------------------------------
// createUploadBuffer()
// Creates a buffer in UPLOAD memory so the CPU can write to it directly.
// This buffer remains in CPU-accessible memory (UPLOAD heap).
// ----------------------------------------------------------------------------
ComPtr<ID3D12Resource> ResourcesModule::createUploadBuffer(const void* data, size_t size, const char* name)
{
	D3D12Module* d3d12 = app->getD3D12();
	ID3D12Device2* device = d3d12->getDevice();
	//ID3D12CommandQueue* queue = d3d12->getCommandQueue();  <<<<<<  Not used

	ComPtr<ID3D12Resource> buffer;


	// -----------------------------------------------------------------
	// --- DESCRIBE THE RAW BUFFER ---
	// -----------------------------------------------------------------
	D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(size);                         // Defines a simple linear buffer of 'size' bytes.

	// -----------------------------------------------------------------
    // --- SPECIFY UPLOAD HEAP (CPU-WRITABLE MEMORY) ---
    // ------------------------------------------------------------------
	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);                              // UPLOAD heap allows direct CPU access and is backed by write-combined memory.


	// -----------------------------------------------------------------
   // --- 3) CREATE THE UPLOAD BUFFER RESOURCE ---
   // -----------------------------------------------------------------
	device->CreateCommittedResource                                                         // We keep it in D3D12_RESOURCE_STATE_COMMON because we only map/write it.
	(&heapProps, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&buffer)); 

	// -----------------------------------------------------------------
	// --- CPU: MAP AND WRITE DATA ---
	// -----------------------------------------------------------------
	BYTE* pData = nullptr;
	CD3DX12_RANGE readRange(0, 0);                                                         // CPU will NOT read from this resource, so range is (0,0)

	buffer->Map(0, &readRange, reinterpret_cast<void**>(&pData));                          // Get CPU pointer
	memcpy(pData, data, size);                                                             // Copy CPU → GPU upload heap
	buffer->Unmap(0, nullptr);                                                             // Invalidate CPU pointer

	return buffer;
}

// ---------------------------------------------------------------------------
// createDefaultBuffer()
// Creates a DEFAULT heap buffer (GPU-only memory) and uploads data to it using
// an intermediate UPLOAD buffer.
// This is the correct way to create GPU-optimized resources in D3D12.
// ----------------------------------------------------------------------------
ComPtr<ID3D12Resource> ResourcesModule::createDefaultBuffer(const void* data, size_t size, const char* name)
{
	D3D12Module* d3d12 = app->getD3D12();
	ID3D12Device2* device = d3d12->getDevice();
	ID3D12CommandQueue* queue = d3d12->getCommandQueue();

	ComPtr<ID3D12Resource> vertexBuffer;
	ComPtr<ID3D12Resource> stagingBuffer;

	// -----------------------------------------------------------------
	// --- THE FINAL GPU BUFFER (DEFAULT HEAP) ---
	// -----------------------------------------------------------------
	auto defaultHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(size);
	device->CreateCommittedResource(&defaultHeap, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&vertexBuffer));

	// -----------------------------------------------------------------
	// --- THE STAGING BUFFER (UPLOAD HEAP) ---
	// -----------------------------------------------------------------
	auto uploadHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	device->CreateCommittedResource(&uploadHeap, D3D12_HEAP_FLAG_NONE,&bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&stagingBuffer));

	// -----------------------------------------------------------------
	// --- CPU: WRITE STAGING BUFFER ---
	// -----------------------------------------------------------------
	BYTE* pData = nullptr;
	CD3DX12_RANGE readRange(0, 0);                                         // We won't read from it, so range is (0,0)
	stagingBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pData));
	memcpy(pData, data, size);                                             // Copy our application data into the GPU buffer
	stagingBuffer->Unmap(0, nullptr);                                      // Unmap the buffer (invalidate the pointer)

	// -----------------------------------------------------------------
	// ---  RECORD COMMANDS & GPU: COPY DATA ---
	// -----------------------------------------------------------------
	commandAllocator->Reset();
	commandList->Reset(commandAllocator.Get(), nullptr);
	commandList->CopyResource(vertexBuffer.Get(), stagingBuffer.Get());    // GPU copy data
	commandList->Close();

	// ----------------------------------------------------------------
	// --- EXECUTE ---
	// ----------------------------------------------------------------
	ID3D12CommandList* lists[] = { commandList.Get() };
	queue->ExecuteCommandLists(_countof(lists), lists);

	// ----------------------------------------------------------------
	// --- WAIT ---
	// ----------------------------------------------------------------
	d3d12->waitForGPU();

	return vertexBuffer;
}

ComPtr<ID3D12Resource> ResourcesModule::createRawTexture2D(const void* data, size_t rowSize, size_t width, size_t height, DXGI_FORMAT format)
{
	return ComPtr<ID3D12Resource>();
}

ComPtr<ID3D12Resource> ResourcesModule::createTextureFromMemory(const void* data, size_t size, const char* name)
{
	return ComPtr<ID3D12Resource>();
}

ComPtr<ID3D12Resource> ResourcesModule::createTextureFromFile(const std::filesystem::path& path, bool defaultSRGB)
{
	const wchar_t* fileName = path.c_str();
	ScratchImage image;
	bool ok = SUCCEEDED(LoadFromDDSFile(fileName, DDS_FLAGS_NONE, nullptr, image));
	ok = ok || SUCCEEDED(LoadFromHDRFile(fileName, nullptr, image));
	ok = ok || SUCCEEDED(LoadFromTGAFile(fileName, defaultSRGB ? TGA_FLAGS_DEFAULT_SRGB : TGA_FLAGS_NONE, nullptr, image));
	ok = ok || SUCCEEDED(LoadFromWICFile(fileName, defaultSRGB ? DirectX::WIC_FLAGS_DEFAULT_SRGB : DirectX::WIC_FLAGS_NONE, nullptr, image));

	if (ok)
	{
		return createTextureFromImage(image, path.string().c_str());
	}

	Logger::Warn("ResourceModule::createTextureFromFile() couldn't create the texture.");

	return nullptr;
}

ComPtr<ID3D12Resource> ResourcesModule::createTextureFromImage(const ScratchImage& image, const char* name)
{
	D3D12Module* d3d12 = app->getD3D12();
	ID3D12Device2* device = d3d12->getDevice();

	ComPtr<ID3D12Resource> texture;
	const TexMetadata& metaData = image.GetMetadata();

	_ASSERTE(metaData.dimension == TEX_DIMENSION_TEXTURE2D);

	if (metaData.dimension == TEX_DIMENSION_TEXTURE2D)
	{
		D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Tex2D(metaData.format, UINT64(metaData.width), UINT(metaData.height),
			UINT16(metaData.arraySize), UINT16(metaData.mipLevels));

		CD3DX12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		bool ok = SUCCEEDED(device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc,
			D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&texture)));

		ComPtr<ID3D12Resource> upload;
		if (ok)
		{
			_ASSERTE(metaData.mipLevels * metaData.arraySize == image.GetImageCount());
			upload = getUploadHeap(GetRequiredIntermediateSize(texture.Get(), 0, UINT(image.GetImageCount())));
			ok = upload != nullptr;
		}

		if (ok)
		{
			std::vector<D3D12_SUBRESOURCE_DATA> subData;
			subData.reserve(image.GetImageCount());

			for (size_t item = 0; item < metaData.arraySize; ++item)
			{
				for (size_t level = 0; level < metaData.mipLevels; ++level)
				{
					const DirectX::Image* subImg = image.GetImage(level, item, 0);

					D3D12_SUBRESOURCE_DATA data = { subImg->pixels, (LONG_PTR)subImg->rowPitch, (LONG_PTR)subImg->slicePitch };

					subData.push_back(data);
				}
			}

			ok = UpdateSubresources(commandList.Get(), texture.Get(), upload.Get(), 0, 0, UINT(image.GetImageCount()), subData.data()) != 0;
		}

		if (ok)
		{
			CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(texture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			commandList->ResourceBarrier(1, &barrier);
			commandList->Close();

			ID3D12CommandList* commandLists[] = { commandList.Get() };
			ID3D12CommandQueue* queue = d3d12->getCommandQueue();

			queue->ExecuteCommandLists(UINT(std::size(commandLists)), commandLists);

			d3d12->waitForGPU();

			commandAllocator->Reset();
			ok = SUCCEEDED(commandList->Reset(commandAllocator.Get(), nullptr));

			texture->SetName(std::wstring(name, name + strlen(name)).c_str());
			return texture;
		}
	}

	return ComPtr<ID3D12Resource>();
}

ComPtr<ID3D12Resource> ResourcesModule::getUploadHeap(size_t size)
{
	D3D12Module* d3d12 = app->getD3D12();
	ID3D12Device* device = d3d12->getDevice();

	ComPtr<ID3D12Resource> uploadHeap;

	CD3DX12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	CD3DX12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(size);
	device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&uploadHeap));

	return uploadHeap;
}

