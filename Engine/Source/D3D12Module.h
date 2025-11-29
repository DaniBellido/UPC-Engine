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
	ComPtr<IDXGISwapChain3> swapChain;
	ComPtr<ID3D12Resource> backBuffers[FRAMES_IN_FLIGHT];
	ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap;
	ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap;
	ComPtr<ID3D12Resource> depthStencilBuffer;

	ComPtr<ID3D12CommandQueue> commandQueue;
	ComPtr<ID3D12CommandAllocator> commandAllocator[FRAMES_IN_FLIGHT];
	ComPtr<ID3D12GraphicsCommandList> commandList;

	ComPtr<ID3D12Fence> fence;
	HANDLE fenceEvent = nullptr;
	unsigned fenceCounter = 0;
	unsigned fenceValue[FRAMES_IN_FLIGHT] = { 0,0,0 };

	unsigned frameValues[FRAMES_IN_FLIGHT] = { 0,0,0 };
	unsigned frameIndex = 0;
	unsigned lastCompletedFrame = 0;

	bool allowTearing = false;
	bool supportsRT = false;
	unsigned currentBackBufferIdx = 0;

	unsigned windowWidth = 0;
	unsigned windowHeight = 0;
	bool fullscreen = false;
	RECT lastWindowRect;
	

public:
	D3D12Module(HWND hWnd);
	~D3D12Module();

	bool init() override;
	bool cleanUp() override;
	void preRender() override;
	void postRender() override;


	void resize();
	void toogleFullscreen();
	void waitForGPU();

	HWND getHWnd() { return hWnd; }
	IDXGISwapChain3* getSwapChain() { return swapChain.Get(); }
	ID3D12Device5* getDevice() { return device.Get(); }
	ID3D12GraphicsCommandList* getCommandList() { return commandList.Get(); }
	ID3D12CommandAllocator* getCommandAllocator() { return commandAllocator[currentBackBufferIdx].Get(); }
	ID3D12Resource* getBackBuffer() { return backBuffers[currentBackBufferIdx].Get(); }
	ID3D12CommandQueue* getCommandQueue() { return commandQueue.Get(); }

	unsigned getCurrentBackBufferIdx() const { return currentBackBufferIdx; }

	D3D12_CPU_DESCRIPTOR_HANDLE getRenderTargetDescriptor();
	D3D12_CPU_DESCRIPTOR_HANDLE getDepthStencilDescriptor();

	UINT signalDrawQueue();

	unsigned getCurrentFrame() const { return frameIndex; }
	unsigned getLastCompletedFrame() const { return lastCompletedFrame; }

	unsigned getWindowWidth() const { return windowWidth; }
	unsigned getWindowHeight() const { return windowHeight; }

	ID3D12GraphicsCommandList* beginFrameRender();
	void setBackBufferRenderTarget(const Vector4& clearColor = Vector4(0.0f, 0.0f, 0.0f, 1.0f));
	void endFrameRender();

	/*ID3D12DescriptorHeap* getRtvDescriptorHeap() { return rtvDescriptorHeap.Get(); }
	ID3D12DescriptorHeap* getDsvDescriptorHeap() { return dsvDescriptorHeap.Get(); }*/

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
	bool createDepthStencil();
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

