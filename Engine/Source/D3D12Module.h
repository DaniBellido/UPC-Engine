#pragma once
#include "Module.h"
#include "dxgi1_6.h"

class D3D12Module : public Module
{
private:
	ComPtr<IDXGIFactory6> factory;
	ComPtr<IDXGIAdapter4> adapter;
	ComPtr<ID3D12Device5> device;

	ComPtr<ID3D12GraphicsCommandList> commandList;
	ComPtr<ID3D12CommandQueue> commandQueue;
	ComPtr<ID3D12CommandAllocator> commandAllocator;


public:
	D3D12Module() {}
	~D3D12Module() {}

	bool init() override;
	bool cleanUp() override;

	void enableDebugLayer();
	void createDevice();
	void createCommandAllocator();
	void createCommandList();
	void setUpInfoQueue();


};

