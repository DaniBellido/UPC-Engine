#pragma once
#include "Module.h"
#include "dxgi1_6.h"

class D3D12Module : public Module
{
private:
	HWND hWnd = NULL;
	ComPtr<IDXGIFactory6> factory;
	ComPtr<IDXGIAdapter4> adapter;
	ComPtr<ID3D12Device5> device;

	ComPtr<ID3D12GraphicsCommandList> commandList;
	ComPtr<ID3D12CommandQueue> commandQueue;
	ComPtr<ID3D12CommandAllocator> commandAllocator;

	ComPtr<IDXGISwapChain> swapChain;

	unsigned windowWidth = 0;
	unsigned windowHeight = 0;


public:
	D3D12Module(HWND hWnd);
	~D3D12Module() {}

	bool init() override;
	bool cleanUp() override;

	void enableDebugLayer();
	void createDevice();
	void createCommandAllocator();
	void createCommandList();
	void createCommandQueue();
	void executeCommandList();
	void setUpInfoQueue();
	void createSwapChain();


};

