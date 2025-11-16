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
	D3D12Module* d3d12 = app->getD3D12();
	ID3D12Device4* device = d3d12->getDevice();

	device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator));
	device->CreateCommandList1(0, D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(&commandList));
	commandList->Reset(commandAllocator.Get(), nullptr);
	commandList->Close();

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
