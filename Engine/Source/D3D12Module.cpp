#include "Globals.h"
#include "D3D12Module.h"

D3D12Module::D3D12Module(HWND wnd) : hWnd(wnd)
{
	currentBackBufferIdx = 0;
}

bool D3D12Module::init()
{
	// DebugLayer
#if defined(_DEBUG)
	enableDebugLayer();
#endif

	// Create Device
	createDevice();

#if defined(_DEBUG)
	setUpInfoQueue();
#endif

	createCommandQueue();
	createSwapChain();
	createCommandAllocator();
	createCommandList();
	createRenderTarget();
	createFence();

	return true;
}

bool D3D12Module::cleanUp()
{
	if (fence && fenceEvent)
	{
		// signal current fence value
		commandQueue->Signal(fence.Get(), fenceValue);
		if (fence->GetCompletedValue() < fenceValue)
		{
			fence->SetEventOnCompletion(fenceValue, fenceEvent);
			WaitForSingleObject(fenceEvent, INFINITE);
		}
	}

	if (fenceEvent)
		CloseHandle(fenceEvent);

	return true;
}

void D3D12Module::enableDebugLayer()
{
	ComPtr<ID3D12Debug> debugInterface;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface))))
	{
		debugInterface->EnableDebugLayer();
	}
}

void D3D12Module::createDevice()
{
#if defined(_DEBUG)
	ThrowIfFailed(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&factory)));
#else
	ThrowIfFailed(CreateDXGIFactory2(0, IID_PPV_ARGS(&factory)));
#endif

	// Choose adapter
	ThrowIfFailed(factory->EnumAdapterByGpuPreference(0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter)));

	// Create device on that adapter
	ThrowIfFailed(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&device)));
}

void D3D12Module::createCommandQueue()
{
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.NodeMask = 0;

	ThrowIfFailed(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue)));
}

void D3D12Module::createCommandAllocator()
{
	for (UINT i = 0; i < FRAMES_IN_FLIGHT; ++i)
	{
		ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator[i])));
	}
}

void D3D12Module::createCommandList()
{
	// create an initially-closed command list using allocator 0
	ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator[0].Get(), nullptr, IID_PPV_ARGS(&commandList)));
	// command lists are created in the recording state; close it so the first Reset is explicit at frame start
	ThrowIfFailed(commandList->Close());
}

void D3D12Module::executeCommandList()
{
	ID3D12CommandList* list[] = { commandList.Get() };
	commandQueue->ExecuteCommandLists(_countof(list), list);
}

void D3D12Module::setUpInfoQueue()
{
	ComPtr<ID3D12InfoQueue> infoQueue;
	if (SUCCEEDED(device.As(&infoQueue)))
	{
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);
	}
}

void D3D12Module::createSwapChain()
{
	// Fetch client rect to get a non-zero width/height
	RECT rc;
	if (GetClientRect(hWnd, &rc))
	{
		windowWidth = std::max(1u, static_cast<unsigned>(rc.right - rc.left));
		windowHeight = std::max(1u, static_cast<unsigned>(rc.bottom - rc.top));
	}
	else
	{
		// fallback reasonable defaults
		windowWidth = 1280;
		windowHeight = 720;
	}

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.Width = windowWidth;
	swapChainDesc.Height = windowHeight;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.Stereo = FALSE;
	swapChainDesc.SampleDesc = { 1, 0 };
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = FRAMES_IN_FLIGHT;
	swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	swapChainDesc.Flags = 0;

	ComPtr<IDXGISwapChain1> swapChain1;

	// CreateSwapChainForHwnd expects an IDXGIFactory (not factory.Get()) and the commandQueue (IUnknown*)
	// Use AddressOf() on the ComPtr when passing an out parameter
	HRESULT hr = factory->CreateSwapChainForHwnd(
		commandQueue.Get(),
		hWnd,
		&swapChainDesc,
		nullptr,
		nullptr,
		&swapChain1
	);
	ThrowIfFailed(hr);

	// Disable Alt+Enter fullscreen toggle if you don't want it (optional)
	factory->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER);

	// Convert to IDXGISwapChain3
	ThrowIfFailed(swapChain1.As(&swapChain));

	// Set current back buffer index
	currentBackBufferIdx = swapChain->GetCurrentBackBufferIndex();
}

void D3D12Module::preRender()
{
	// At the start of a frame we must reset allocator and command list for the current back buffer
	currentBackBufferIdx = swapChain->GetCurrentBackBufferIndex();

	// Reset allocator for this frame
	ThrowIfFailed(commandAllocator[currentBackBufferIdx]->Reset());

	// Reset the command list to start recording commands for this frame
	ThrowIfFailed(commandList->Reset(commandAllocator[currentBackBufferIdx].Get(), nullptr));

	// Get the back buffer resource for this index
	ThrowIfFailed(swapChain->GetBuffer(currentBackBufferIdx, IID_PPV_ARGS(&backBuffers[currentBackBufferIdx])));

	// Transition from PRESENT -> RENDER_TARGET
	D3D12_RESOURCE_BARRIER barrierToRender = CD3DX12_RESOURCE_BARRIER::Transition(
		backBuffers[currentBackBufferIdx].Get(),
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET
	);
	commandList->ResourceBarrier(1, &barrierToRender);

	// Set the RTV (and optionally DSV) so ClearRenderTargetView affects current buffer
	auto rtvHandle = getRenderTargetDescriptor();
	commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
}

void D3D12Module::render()
{
	// Get render descriptor
	auto rtvHandle = getRenderTargetDescriptor();

	// Red RGBA (1.0f, 0.0f, 0.0f, 1.0f)
	const float clearColor[] = { 1.0f, 0.0f, 0.0f, 1.0f };

	// Clear the current RTV
	commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
}

void D3D12Module::postRender()
{
	// Transition RENDER_TARGET -> PRESENT
	D3D12_RESOURCE_BARRIER barrierToPresent = CD3DX12_RESOURCE_BARRIER::Transition(
		backBuffers[currentBackBufferIdx].Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT
	);
	commandList->ResourceBarrier(1, &barrierToPresent);

	// Close and execute
	ThrowIfFailed(commandList->Close());
	ID3D12CommandList* lists[] = { commandList.Get() };
	commandQueue->ExecuteCommandLists(_countof(lists), lists);

	// Present
	ThrowIfFailed(swapChain->Present(1, 0));

	// Signal and wait with fence to ensure GPU finished the work for this frame.
	// Use current fenceValue then increment.
	ThrowIfFailed(commandQueue->Signal(fence.Get(), fenceValue));

	if (fence->GetCompletedValue() < fenceValue)
	{
		ThrowIfFailed(fence->SetEventOnCompletion(fenceValue, fenceEvent));
		WaitForSingleObject(fenceEvent, INFINITE);
	}
	fenceValue++;

	// After present, update the back buffer index for next frame (GetCurrentBackBufferIndex reflects next index)
	currentBackBufferIdx = swapChain->GetCurrentBackBufferIndex();
}

void D3D12Module::createRenderTarget()
{
	// RTV heap
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors = FRAMES_IN_FLIGHT;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	ThrowIfFailed(device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvDescriptorHeap)));

	// Tamaño del incremento entre descriptores
	UINT rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	for (UINT i = 0; i < FRAMES_IN_FLIGHT; ++i)
	{
		ThrowIfFailed(swapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffers[i])));
		device->CreateRenderTargetView(backBuffers[i].Get(), nullptr, rtvHandle);
		rtvHandle.Offset(1, rtvDescriptorSize);
	}
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12Module::getRenderTargetDescriptor()
{
	UINT rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	rtvHandle.ptr += static_cast<SIZE_T>(currentBackBufferIdx) * rtvDescriptorSize;
	return rtvHandle;
}

void D3D12Module::createFence()
{
	ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
	fenceValue = 1;
	fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (fenceEvent == nullptr)
	{
		ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
	}
}
