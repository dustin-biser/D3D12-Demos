#include "pch.h"
#include "D3D12DemoBase.hpp"

using namespace Microsoft::WRL;


//---------------------------------------------------------------------------------------
// Helper function for acquiring the first available hardware adapter that supports
// the given feature level. If no such adapter can be found, *ppAdapter will be set to
// nullptr.
static void GetHardwareAdapter (
	_In_ IDXGIFactory2 * pFactory,
	_In_ D3D_FEATURE_LEVEL featureLevel,
	_Out_ IDXGIAdapter1 ** ppAdapter
) {
	ComPtr<IDXGIAdapter1> adapter;
	*ppAdapter = nullptr;

	uint adapterIndex(0);
	while (DXGI_ERROR_NOT_FOUND != pFactory->EnumAdapters1(adapterIndex, &adapter)) {
		DXGI_ADAPTER_DESC1 desc;
		adapter->GetDesc1(&desc);

		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
			// Don't select the Basic Render Driver adapter.
			// If you want a software adapter, pass in "/warp" on the command line.
			continue;
		}

		// Check to see if the adapter supports Direct3D 12, but don't create the
		// actual device yet.
		if (SUCCEEDED(D3D12CreateDevice(adapter.Get(),
				featureLevel, _uuidof(ID3D12Device), nullptr))) {
			break;
		}

		++adapterIndex;
	}

	// Return first hardware adapter found, that supports the specified D3D feature set.
	// Call Detach() so that the ComPtr does not destroy the interface object when exiting
	// the current scope.
	*ppAdapter = adapter.Detach();
}


//---------------------------------------------------------------------------------------
void D3D12DemoBase::CreateHardwareDevice (
	IDXGIFactory4 * dxgiFactory,
	D3D_FEATURE_LEVEL featureLevel
) {
	ComPtr<IDXGIAdapter1> hardwareAdapter;
	::GetHardwareAdapter(dxgiFactory, featureLevel, &hardwareAdapter);

	if (!hardwareAdapter) {
		ForceBreak("No hardware adapter found that supports the given feature level");
	}

	// Display hardware adapter name.
	DXGI_ADAPTER_DESC1 adapterDesc = {};
	hardwareAdapter->GetDesc1(&adapterDesc);
	std::wcout << L"Adapter: " << adapterDesc.Description << std::endl;

	CHECK_D3D_RESULT (
		D3D12CreateDevice(hardwareAdapter.Get(), featureLevel, IID_PPV_ARGS(&m_device))
	);
	SET_D3D12_DEBUG_NAME(m_device);
}

//---------------------------------------------------------------------------------------
void D3D12DemoBase::CreateDeviceAndSwapChain()
{
	IDXGIFactory4 * dxgiFactory;

	CHECK_D3D_RESULT (
		::CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory))
	);

	// Prevent full screen transitions for now.
	CHECK_D3D_RESULT (
		dxgiFactory->MakeWindowAssociation (
			Win32Application::GetHwnd(),
			DXGI_MWA_NO_ALT_ENTER
		)
	);


	CreateHardwareDevice(dxgiFactory, D3D_FEATURE_LEVEL_11_0);


	// Describe and create the direct command queue.
	{
		D3D12_COMMAND_QUEUE_DESC queueDesc = {};
		queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		CHECK_D3D_RESULT (
			m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_directCmdQueue))
		);
		SET_D3D12_DEBUG_NAME(m_directCmdQueue);
	}


	// Describe and create the swap chain.
	{
		DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
		swapChainDesc.BufferCount = NUM_BUFFERED_FRAMES;
		swapChainDesc.Width = m_windowWidth;
		swapChainDesc.Height = m_windowHeight;
		swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;

		ComPtr<IDXGISwapChain1> swapChain1;
		CHECK_D3D_RESULT (
			dxgiFactory->CreateSwapChainForHwnd (
				m_directCmdQueue.Get(), // Swap chain needs the command queue so that 
				                        // it can force a flush on it.
				Win32Application::GetHwnd(),
				&swapChainDesc,
				nullptr,
				nullptr,
				&swapChain1
			)
		);

		ComPtr<IDXGISwapChain2> swapChain2;
		CHECK_D3D_RESULT (
			swapChain1.As(&swapChain2)
		);
		swapChain2->SetMaximumFrameLatency(NUM_BUFFERED_FRAMES);

		// Acquire handle to frame latency waitable object.
		m_frameLatencyWaitableObject = swapChain2->GetFrameLatencyWaitableObject();

		// Assign interface object to m_swapChain so it persists past current scope.
		CHECK_D3D_RESULT (
			swapChain1.As(&m_swapChain)
		);
	}

	// Set the current frame index to correspond with the current back buffer index.
	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
}

//---------------------------------------------------------------------------------------
void WaitForGpuFence (
	ID3D12Fence * fence,
	uint64 completionValue,
	HANDLE fenceEvent
) {
	if (fence->GetCompletedValue() < completionValue) {
		CHECK_D3D_RESULT (
			fence->SetEventOnCompletion(completionValue, fenceEvent)
		);
		// fenceEvent will be signaled once GPU updates fence object to completionValue.
		WaitForSingleObject(fenceEvent, INFINITE);
	}
}


//---------------------------------------------------------------------------------------
D3D12DemoBase::D3D12DemoBase (
	uint windowWidth,
	uint windowHeight,
	std::wstring windowTitle
) :
	m_frameIndex(0),
	m_windowWidth(windowWidth),
	m_windowHeight(windowHeight),
	m_windowTitle(windowTitle),
	m_fenceValue{0}
{
	// Default viewport to size of full window.
	m_viewport.Width = static_cast<float>(windowWidth);
	m_viewport.Height = static_cast<float>(windowHeight);
	m_viewport.MaxDepth = 1.0f;

	// Default to rendering to entire viewport.
	m_scissorRect.right = static_cast<long>(windowWidth);
	m_scissorRect.bottom = static_cast<long>(windowHeight);

    //-- Set working directory path:
    {
        WCHAR pathBuffer[512];
        GetWorkingDir(pathBuffer, _countof(pathBuffer));
        m_workingDirPath = std::wstring(pathBuffer);
    }

    //-- Set shared asset path:
    {
        WCHAR pathBuffer[512];
        GetSolutionDir(pathBuffer, _countof(pathBuffer));
        m_sharedAssetPath = std::wstring(pathBuffer) + L"Assets\\";
    }

	// Check for DirectXMath support.
	if (!DirectX::XMVerifyCPUSupport()) {
		ForceBreak("No support for DirectXMath.");
	}

	// Initialize COM library.
	CHECK_WIN_RESULT (
		CoInitializeEx(nullptr, COINIT_MULTITHREADED)
	);
}

//---------------------------------------------------------------------------------------
void D3D12DemoBase::CreateDirectCommandQueue ()
{
}

//---------------------------------------------------------------------------------------
D3D12DemoBase::~D3D12DemoBase()
{
	// Clean up event handles
	for (auto event : m_frameFenceEvent) {
		CloseHandle(event);
	}

	// Uninitialize COM library.
	CoUninitialize();
}

//---------------------------------------------------------------------------------------
void D3D12DemoBase::Initialize()
{
#if defined(_DEBUG)
	// Enable the D3D12 debug layer.
	ComPtr<ID3D12Debug> debugController;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
		debugController->EnableDebugLayer();
	}
#endif

	CreateDeviceAndSwapChain();

	CreateDrawCommandLists();

	CreateDepthStencilBuffer();

	CreateRenderTargetViews();

	CreateFenceObjects();

	ComPtr<ID3D12CommandAllocator> cmdAllocator;
	ComPtr<ID3D12GraphicsCommandList> uploadCmdList;
	GenerateCommandList(uploadCmdList, cmdAllocator);

	// Setup derived demo.
	InitializeDemo(uploadCmdList.Get());

	// Close command list and execute it on the direct command queue. 
	CHECK_D3D_RESULT (
		uploadCmdList->Close()
	);
	ID3D12CommandList * commandLists[] = { uploadCmdList.Get() };
	m_directCmdQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

	WaitForGpuCompletion(m_directCmdQueue.Get());
}


//---------------------------------------------------------------------------------------
void D3D12DemoBase::CreateFenceObjects()
{
	// Create synchronization primitives.
	for (int i(0); i < NUM_BUFFERED_FRAMES; ++i) {
		CHECK_D3D_RESULT(
			m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_frameFence[i]))
		);
		m_fenceValue[i] = 0;

		// Create an event handle to use for frame synchronization.
		m_frameFenceEvent[i] = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (m_frameFenceEvent[i] == nullptr) {
			CHECK_D3D_RESULT(
				HRESULT_FROM_WIN32(GetLastError())
			);
		}
	}

	// Initialize incremental fence value.
	m_currentFenceValue = 1;
}


//---------------------------------------------------------------------------------------
void D3D12DemoBase::CreateRenderTargetViews()
{
	//-- Describe and create the RTV Descriptor Heap.
	{
		D3D12_DESCRIPTOR_HEAP_DESC rtvDescHeapDescriptor = {};

		// The RTV Descriptor Heap will hold a RTV Descriptor for each swap chain buffer.
		rtvDescHeapDescriptor.NumDescriptors = NUM_BUFFERED_FRAMES;
		rtvDescHeapDescriptor.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvDescHeapDescriptor.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		CHECK_D3D_RESULT (
			m_device->CreateDescriptorHeap(&rtvDescHeapDescriptor, IID_PPV_ARGS(&m_rtvDescHeap))
		);
	}

	//-- Create a RTV for each swap-chain buffer.
	{
		// Specify a RTV with sRGB format to support gamma correction.
		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

		// Get increment size between descriptors in RTV Descriptor Heap.
		uint handleIncrementSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		// Create a render target view for each frame.
		for (uint n(0); n < NUM_BUFFERED_FRAMES; ++n) {
			CHECK_D3D_RESULT (
				m_swapChain->GetBuffer(n, IID_PPV_ARGS(&m_renderTarget[n].resource))
			);

			m_renderTarget[n].rtvHandle = m_rtvDescHeap->GetCPUDescriptorHandleForHeapStart();

			// Offset handle to next descriptor location within descriptor heap.
			m_renderTarget[n].rtvHandle.ptr += n * handleIncrementSize;

			// Create RTV and store its descriptor at the heap location reference by
			// the descriptor handle.
			m_device->CreateRenderTargetView (
				m_renderTarget[n].resource, &rtvDesc, m_renderTarget[n].rtvHandle
			);
			SET_D3D12_DEBUG_NAME(m_renderTarget[n].resource);
		}
	}
}

//---------------------------------------------------------------------------------------
void D3D12DemoBase::CreateDepthStencilBuffer()
{
	// create a depth stencil descriptor heap so we can get a pointer to the depth stencil buffer
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDescriptor = {};
	dsvHeapDescriptor.NumDescriptors = 1;
	dsvHeapDescriptor.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDescriptor.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	CHECK_D3D_RESULT (
		m_device->CreateDescriptorHeap(&dsvHeapDescriptor, IID_PPV_ARGS(&m_dsvDescHeap))
	);
	SET_D3D12_DEBUG_NAME(m_dsvDescHeap);

	D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
	depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;
	depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;

	D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
	depthOptimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
	depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
	depthOptimizedClearValue.DepthStencil.Stencil = 0;

	// Create Depth-Stencil Buffer resource.
	CHECK_D3D_RESULT(
		m_device->CreateCommittedResource (
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, m_windowWidth, m_windowHeight,
				1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL),
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			&depthOptimizedClearValue,
			IID_PPV_ARGS(&m_depthStencilBuffer.resource)
		)
	);
	SET_D3D12_DEBUG_NAME(m_depthStencilBuffer.resource);

	m_device->CreateDepthStencilView (
		m_depthStencilBuffer.resource,
		&depthStencilDesc,
		m_dsvDescHeap->GetCPUDescriptorHandleForHeapStart()
	);
}

//---------------------------------------------------------------------------------------
void D3D12DemoBase::CreateDrawCommandLists()
{
	//-- Create command allocator for managing command list memory.
	for (int i(0); i < NUM_BUFFERED_FRAMES; ++i) {
		CHECK_D3D_RESULT(
			m_device->CreateCommandAllocator (
				D3D12_COMMAND_LIST_TYPE_DIRECT,
				IID_PPV_ARGS(&m_directCmdAllocator[i])
			)
		);
	}
	NAME_D3D12_OBJECT_ARRAY(m_directCmdAllocator, NUM_BUFFERED_FRAMES);


	//-- Create the direct command lists which will hold our rendering commands:
	{
		// Create one command list for each swap chain buffer.
		for (uint i(0); i < NUM_BUFFERED_FRAMES; ++i) {
			CHECK_D3D_RESULT(
				m_device->CreateCommandList (
					0,
					D3D12_COMMAND_LIST_TYPE_DIRECT,
					m_directCmdAllocator[i].Get(),
					nullptr, // Will set pipeline state later before drawing
					IID_PPV_ARGS(&m_drawCmdList[i])
				)
			);
			// Stop recording, will reset this later before issuing drawing commands.
			m_drawCmdList[i]->Close();
		}
		NAME_D3D12_OBJECT_ARRAY(m_drawCmdList, NUM_BUFFERED_FRAMES);
	}

}

//---------------------------------------------------------------------------------------
void D3D12DemoBase::GenerateCommandList(
	ComPtr<ID3D12GraphicsCommandList> & commandList,
	ComPtr<ID3D12CommandAllocator> & cmdAllocator
) {
	// Create command allocator.
	CHECK_D3D_RESULT(
		m_device->CreateCommandAllocator (
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			IID_PPV_ARGS(&cmdAllocator)
		)
	);
	SET_D3D12_DEBUG_NAME(cmdAllocator);


	// Create command list.
	CHECK_D3D_RESULT(
		m_device->CreateCommandList (
			0,
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			cmdAllocator.Get(),
			nullptr,
			IID_PPV_ARGS(&commandList)
		)
	);
	SET_D3D12_DEBUG_NAME(commandList);
}


//---------------------------------------------------------------------------------------
void D3D12DemoBase::BuildNextFrame()
{
	// Wait until GPU has processed the previous indexed frame before building new one.
	::WaitForGpuFence (
		m_frameFence[m_frameIndex].Get(),
		m_fenceValue[m_frameIndex],
		m_frameFenceEvent[m_frameIndex]
	);

	Update();

	if (m_vsyncEnabled) {
		// Wait until swap chain has finished presenting all queued frames before building
		// command lists and rendering next frame.  This will reduce latency for the next
		// rendered frame.
		WaitForSingleObject(m_frameLatencyWaitableObject, INFINITE);
	}

	// Acquire commandList corresponding to current frame index.
	auto drawCmdList = m_drawCmdList[m_frameIndex].Get();

	PrepareRender(m_directCmdAllocator[m_frameIndex].Get(), drawCmdList);

	Render(drawCmdList);

	FinalizeRender(drawCmdList, m_directCmdQueue.Get());
}

//---------------------------------------------------------------------------------------
void D3D12DemoBase::PresentNextFrame()
{
	if (m_vsyncEnabled) {
		Present();

	} else if (SwapChainWaitableObjectIsSignaled()) {
		// Swap-chain is available to queue up another Present, so do that now.
		Present();
	}
	else { 
		// Start over and rebuild the frame again, rendering to the same indexed back buffer.
		m_directCmdQueue->Signal(m_frameFence[m_frameIndex].Get(), m_currentFenceValue);
		m_fenceValue[m_frameIndex] = m_currentFenceValue;
		++m_currentFenceValue;
	}
}

//---------------------------------------------------------------------------------------
// Present contents of back-buffer, signal the frame-fence, then increment
// frame index so the next frame can be processed.
void D3D12DemoBase::Present()
{
	const uint syncInterval = m_vsyncEnabled;
	CHECK_D3D_RESULT (
		m_swapChain->Present(syncInterval, 0)
	);

	m_directCmdQueue->Signal(m_frameFence[m_frameIndex].Get(), m_currentFenceValue);
	m_fenceValue[m_frameIndex] = m_currentFenceValue;
	++m_currentFenceValue;

	m_frameIndex = (m_frameIndex + 1) % NUM_BUFFERED_FRAMES;
}

//---------------------------------------------------------------------------------------
__forceinline bool D3D12DemoBase::SwapChainWaitableObjectIsSignaled()
{
	return WAIT_OBJECT_0 == WaitForSingleObjectEx(m_frameLatencyWaitableObject, 0, true);
}

//---------------------------------------------------------------------------------------
void D3D12DemoBase::WaitForGpuCompletion (
	ID3D12CommandQueue * commandQueue
) {
	CHECK_D3D_RESULT (
		commandQueue->Signal(m_frameFence[m_frameIndex].Get(), m_currentFenceValue)
	);
	m_fenceValue[m_frameIndex] = m_currentFenceValue;
	++m_currentFenceValue;

	::WaitForGpuFence(m_frameFence[m_frameIndex].Get(),
		m_fenceValue[m_frameIndex], m_frameFenceEvent[m_frameIndex]);
}

//---------------------------------------------------------------------------------------
void D3D12DemoBase::OnKeyDown(uint8 key)
{
	// Empty, to be overridden by derived class
}

//---------------------------------------------------------------------------------------
void D3D12DemoBase::OnKeyUp(uint8 key)
{
	// Empty, to be overridden by derived class
}

//---------------------------------------------------------------------------------------
void D3D12DemoBase::OnResize(uint windowWidth, uint windowHeight)
{
	m_windowWidth = windowWidth;
	m_windowHeight = windowHeight;
}

//---------------------------------------------------------------------------------------
void D3D12DemoBase::OnMouseMove(
	int dx,
	int dy
) {

}

//---------------------------------------------------------------------------------------
void D3D12DemoBase::UpdateMousePosition (
	uint x, 
	uint y
) {
	m_mousePosition.x = x;
	m_mousePosition.y = y;
}

//---------------------------------------------------------------------------------------
void D3D12DemoBase::MouseLButtonDown()
{
	m_mouseLButtonDown = true;
}

//---------------------------------------------------------------------------------------
void D3D12DemoBase::MouseLButtonUp()
{
	m_mouseLButtonDown = false;
}

//---------------------------------------------------------------------------------------
void D3D12DemoBase::PrepareRender (
	ID3D12CommandAllocator * commandAllocator,
	ID3D12GraphicsCommandList * drawCmdList
) {
	CHECK_D3D_RESULT (
		commandAllocator->Reset()
	);

	CHECK_D3D_RESULT (
		// Forgo setting pipeline-state for now.
		// Let dervived class set it within its render(...) method.
		drawCmdList->Reset(commandAllocator, nullptr)
	);

	drawCmdList->RSSetViewports(1, &m_viewport);
	drawCmdList->RSSetScissorRects(1, &m_scissorRect);

	// Indicate that the back buffer for the current frame will be used as a render target.
	drawCmdList->ResourceBarrier (1,
		&CD3DX12_RESOURCE_BARRIER::Transition (
			m_renderTarget[m_frameIndex].resource,
			D3D12_RESOURCE_STATE_PRESENT,
			D3D12_RESOURCE_STATE_RENDER_TARGET
		)
	);

	// Acquire handle to Depth-Stencil View.
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle (m_dsvDescHeap->GetCPUDescriptorHandleForHeapStart());

	// Acquire handle to Render Target View.
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle (m_renderTarget[m_frameIndex].rtvHandle);

	drawCmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

	// Clear render target.
	const float clearColor[] = {0.0f, 0.2f, 0.4f, 1.0f};
	drawCmdList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

	// Clear the depth/stencil buffer.
	drawCmdList->ClearDepthStencilView (dsvHandle,
		D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr
	);
}

//---------------------------------------------------------------------------------------
void D3D12DemoBase::FinalizeRender (
	ID3D12GraphicsCommandList * drawCmdList,
	ID3D12CommandQueue * commandQueue
) {
	// Indicate that the back buffer will now be used to present.
	drawCmdList->ResourceBarrier (1,
		&CD3DX12_RESOURCE_BARRIER::Transition (
			m_renderTarget[m_frameIndex].resource,
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PRESENT
		)
	);

	CHECK_D3D_RESULT (
		drawCmdList->Close()
	);

	// Execute the command list.
	ID3D12CommandList* commandLists[] = {drawCmdList};
	commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);
}

//---------------------------------------------------------------------------------------
void D3D12DemoBase::PrepareCleanup()
{
	// Signal command-queue 
	m_directCmdQueue->Signal(m_frameFence[m_frameIndex].Get(), m_currentFenceValue);
	m_fenceValue[m_frameIndex] = m_currentFenceValue;
	++m_currentFenceValue;

	// Wait for command queue to finish processing all buffered frames.
	for (int i(0); i < NUM_BUFFERED_FRAMES; ++i) {
		WaitForGpuFence(m_frameFence[i].Get(), m_fenceValue[i], m_frameFenceEvent[i]);
	}

	// Now it is safe to Release() D3D resources.
}



//---------------------------------------------------------------------------------------
uint D3D12DemoBase::GetWindowWidth() const {
	return m_windowWidth;
}


//---------------------------------------------------------------------------------------
uint D3D12DemoBase::GetWindowHeight() const {
	return m_windowHeight;
}


//---------------------------------------------------------------------------------------
const WCHAR * D3D12DemoBase::GetWindowTitle() const {
	return m_windowTitle.c_str();
}


//---------------------------------------------------------------------------------------
ScreenPosition D3D12DemoBase::GetMousePosition() const
{
	return m_mousePosition;
}

//---------------------------------------------------------------------------------------
std::wstring D3D12DemoBase::GetAssetPath (
	const wchar_t * assetName
) {
    ASSERT(assetName);

    // Compiled shader code .cso files should be in the current working directory,
    // whereas other assets (e.g. textures, meshes, etc.) should be located in
    // the shared asset path.
    const wchar_t * stringMatch = wcsstr(assetName, L".cso");
    if (stringMatch) {
        return m_workingDirPath + assetName;
    }

    return m_sharedAssetPath + assetName;
}


//---------------------------------------------------------------------------------------
std::string D3D12DemoBase::GetAssetPath(const char * assetName)
{
    const int BUFFER_LENGTH = 512;

    wchar_t wcsBuffer[BUFFER_LENGTH]; // wide character string buffer.
    size_t result = mbstowcs(wcsBuffer, assetName, BUFFER_LENGTH);
    if (result == BUFFER_LENGTH) {
        // assetName path is too long to fit in buffer.
        throw;
    }
    std::wstring assetPath = this->GetAssetPath(wcsBuffer);

    char mbsBuffer[BUFFER_LENGTH]; // multi-byte string buffer.
    result = wcstombs(mbsBuffer, assetPath.c_str(), BUFFER_LENGTH);
    if (result == BUFFER_LENGTH) {
        // assetName path is too long to fit in buffer.
        throw;
    }

    return std::string(mbsBuffer);
}

//---------------------------------------------------------------------------------------
// Helper function for setting the window's title text.
void D3D12DemoBase::SetCustomWindowText (
	LPCWSTR text
) {
	std::wstring windowText = m_windowTitle + L": " + text;
	SetWindowText(Win32Application::GetHwnd(), windowText.c_str());
}
