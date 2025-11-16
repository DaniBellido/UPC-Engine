#pragma once
#include "Module.h"
#include "dxgi1_6.h"
#include <stdexcept>

//-----------------------------------------------------------------------------
// D3D12Module manages the Direct3D 12 graphics device and swap chain.
// It handles device creation, resource management, rendering synchronization,
// window resizing, and fullscreen toggling. Provides access to core D3D12
// objects.
//-----------------------------------------------------------------------------

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
	UINT fenceValue[FRAMES_IN_FLIGHT] = { 0,0,0 };
	HANDLE fenceEvent = nullptr;
	unsigned fenceCounter = 0;

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
	ID3D12DescriptorHeap* getRtvDescriptorHeap() { return rtvDescriptorHeap.Get(); }
	ID3D12DescriptorHeap* getDsvDescriptorHeap() { return dsvDescriptorHeap.Get(); }

	D3D12_CPU_DESCRIPTOR_HANDLE getRenderTargetDescriptor();
	D3D12_CPU_DESCRIPTOR_HANDLE getDepthStencilDescriptor();

	void resize();
	void waitForGPU();

	//--------------------------------------------------------
	//PIN8
	unsigned getWindowWidth() const { return windowWidth; }
	unsigned getWindowHeight() const { return windowHeight; }

	ComPtr<ID3D12Resource> sceneRenderTarget;
	ComPtr<ID3D12DescriptorHeap> sceneRTVHeap;
	ComPtr<ID3D12DescriptorHeap> sceneSRVHeap;


	D3D12_GPU_DESCRIPTOR_HANDLE getSceneSRVGpuHandle() const { return sceneSRVHeap->GetGPUDescriptorHandleForHeapStart(); }



private:
	void getWindowSize(unsigned& width, unsigned& height);
	void enableDebugLayer();
	void createDevice();
	void createCommandAllocator();
	void createCommandList();
	void createCommandQueue();
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

