#pragma once
#include "Module.h"
#include "dxgi1_6.h"
#include <stdexcept>

class D3D12Module : public Module
{
private:
	HWND hWnd = NULL;

	// System Setup
	ComPtr<IDXGIFactory6> factory;
	ComPtr<IDXGIAdapter4> adapter;
	ComPtr<ID3D12Device5> device;
	
	// Render Setup
	ComPtr<ID3D12CommandQueue> commandQueue;
	ComPtr<ID3D12CommandAllocator> commandAllocator[FRAMES_IN_FLIGHT];
	ComPtr<ID3D12GraphicsCommandList> commandList;

	ComPtr<IDXGISwapChain3> swapChain;
	ComPtr<ID3D12Resource> backBuffers[FRAMES_IN_FLIGHT];

	ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap;
	ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap;

	ComPtr<ID3D12Fence> fence;
	UINT64 fenceValue = 0;
	HANDLE fenceEvent = nullptr;

	unsigned windowWidth = 0;
	unsigned windowHeight = 0;
	unsigned currentBackBufferIdx = 0;


public:
	D3D12Module(HWND hWnd);
	~D3D12Module() {}

	bool init() override;
	bool cleanUp() override;
	//void update() override;
	void preRender() override;
	void postRender() override;
	void render() override;

	HWND getHWnd() { return hWnd; }
	IDXGISwapChain3* getSwapChain() { return swapChain.Get(); }
	ID3D12Device5* getDevice() { return device.Get(); }
	ID3D12GraphicsCommandList* getCommandList() { return commandList.Get(); }
	ID3D12CommandAllocator* getCommandAllocator() { return commandAllocator[currentBackBufferIdx].Get(); }
	ID3D12Resource* getBackBuffer() { return backBuffers[currentBackBufferIdx].Get(); }
	ID3D12CommandQueue* getCommandQueue() { return commandQueue.Get(); }

	D3D12_CPU_DESCRIPTOR_HANDLE getRenderTargetDescriptor();
	D3D12_CPU_DESCRIPTOR_HANDLE getDepthStencilDescriptor();



private:
	void enableDebugLayer();
	void createDevice();
	void createCommandAllocator();
	void createCommandList();
	void createCommandQueue();
	void executeCommandList();
	void setUpInfoQueue();
	void createSwapChain();
	void createRenderTarget();
	void createDepthStencil();
	void createFence();

	inline void ThrowIfFailed(HRESULT hr)
	{
		if (FAILED(hr))
		{
#if defined(_DEBUG)
			__debugbreak();
#endif
			throw std::runtime_error("HRESULT failed.");
		}
	}


};

