#include "Globals.h"
#include "ResourcesModule.h"
#include "Application.h"

bool ResourcesModule::init()
{
	D3D12Module* d3d12 = app->getD3D12();
	ID3D12Device4* device = d3d12->getDevice();

	device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator));
	device->CreateCommandList1(0, D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(&commandList));
	commandList->Reset(commandAllocator.Get(), nullptr);

	return true;
}

void ResourcesModule::preRender()
{
}

bool ResourcesModule::cleanUp()
{
	return true;
}

ComPtr<ID3D12Resource> ResourcesModule::createUploadBuffer(const void* data, size_t size, const char* name)
{
	D3D12Module* d3d12 = app->getD3D12();
	ID3D12Device2* device = d3d12->getDevice();
	ID3D12CommandQueue* queue = d3d12->getCommandQueue();

	ComPtr<ID3D12Resource> buffer;


	// 1. Describe the buffer
	D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(size);
	// 2. Specify UPLOAD heap properties
	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
	// 3. Create the resource
	device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&buffer));

	BYTE* pData = nullptr;
	CD3DX12_RANGE readRange(0, 0); // We won't read from it, so range is (0,0)
	buffer->Map(0, &readRange, reinterpret_cast<void**>(&pData));

	// Copy our application data into the GPU buffer
	memcpy(pData, data, size);
	// Unmap the buffer (invalidate the pointer)
	buffer->Unmap(0, nullptr);

	return buffer;
}

ComPtr<ID3D12Resource> ResourcesModule::createDefaultBuffer(const void* data, size_t size, const char* name)
{
	D3D12Module* d3d12 = app->getD3D12();
	ID3D12Device2* device = d3d12->getDevice();
	ID3D12CommandQueue* queue = d3d12->getCommandQueue();

	ComPtr<ID3D12Resource> vertexBuffer;
	ComPtr<ID3D12Resource> stagingBuffer;

	// --- CREATE THE FINAL GPU BUFFER(DEFAULT HEAP) -- -
	auto defaultHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(size);
	device->CreateCommittedResource(&defaultHeap, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&vertexBuffer));

	// --- CREATE THE STAGING BUFFER (UPLOAD HEAP) ---
	auto uploadHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	device->CreateCommittedResource(&uploadHeap, D3D12_HEAP_FLAG_NONE,&bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&stagingBuffer));

	// --- CPU: FILL STAGING BUFFER ---
	// // Map the buffer: get a CPU pointer to its memory
	BYTE* pData = nullptr;
	CD3DX12_RANGE readRange(0, 0); // We won't read from it, so range is (0,0)
	stagingBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pData));

	// Copy our application data into the GPU buffer
	memcpy(pData, data, size);
	// Unmap the buffer (invalidate the pointer)
	stagingBuffer->Unmap(0, nullptr);

	commandAllocator->Reset();
	commandList->Reset(commandAllocator.Get(), nullptr);

	// --- GPU: COPY DATA ---
	commandList->CopyResource(vertexBuffer.Get(), stagingBuffer.Get());

	commandList->Close();

	ID3D12CommandList* lists[] = { commandList.Get() };
	queue->ExecuteCommandLists(_countof(lists), lists);

	d3d12->waitForGPU();

	return vertexBuffer;
}
