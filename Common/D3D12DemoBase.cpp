//
// D3D12DemoBase.cpp
//
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
static void CreateWarpDevice (
	_In_ IDXGIFactory4 * dxgiFactory,
	_In_ D3D_FEATURE_LEVEL featureLevel,
	_Out_ Microsoft::WRL::ComPtr<ID3D12Device> & device
) {
	ComPtr<IDXGIAdapter> warpAdapter;
	CHECK_D3D_RESULT (
		dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter))
	);

	// Display warp adapter name.
	DXGI_ADAPTER_DESC adapterDesc = {};
	warpAdapter->GetDesc(&adapterDesc);
	std::wcout << "Adapter: " << adapterDesc.Description << std::endl;

	CHECK_D3D_RESULT (
		D3D12CreateDevice(warpAdapter.Get(), featureLevel, IID_PPV_ARGS(&device))
	);

}

//---------------------------------------------------------------------------------------
static void CreateHardwareDevice (
	_In_ IDXGIFactory4 * dxgiFactory,
	_In_ D3D_FEATURE_LEVEL featureLevel,
	_Out_ Microsoft::WRL::ComPtr<ID3D12Device> & device
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
		D3D12CreateDevice(hardwareAdapter.Get(), featureLevel, IID_PPV_ARGS(&device))
	);
}


//---------------------------------------------------------------------------------------
static void createDevice (
	_In_ IDXGIFactory4 * dxgiFactory,
	_In_ bool useWarpDevice,
	_Out_ Microsoft::WRL::ComPtr<ID3D12Device> & device
) {
	const D3D_FEATURE_LEVEL featureLevel(D3D_FEATURE_LEVEL_11_0);

	if (useWarpDevice) {
		::CreateWarpDevice(dxgiFactory, featureLevel, device);
	}
	else {
		::CreateHardwareDevice(dxgiFactory, featureLevel, device);
	}
}

//---------------------------------------------------------------------------------------
static void createSwapChain (
	_In_ IDXGIFactory4 * dxgiFactory,
	_In_ ID3D12CommandQueue * commandQueue,
	_In_ uint framebufferWidth,
	_In_ uint framebufferHeight,
	_In_ uint numSwapChainBuffers,
	_Out_ ComPtr<IDXGISwapChain3> & swapChain,
	_Out_ HANDLE * frameLatencyWaitableObject
) {
	// Describe and create the swap chain.
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.BufferCount = numSwapChainBuffers;
	swapChainDesc.Width = framebufferWidth;
	swapChainDesc.Height = framebufferHeight;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;

	ComPtr<IDXGISwapChain1> swapChain1;
	CHECK_D3D_RESULT (
		dxgiFactory->CreateSwapChainForHwnd (
			commandQueue, // Swap chain needs the command queue so that it can force a flush on it.
			Win32Application::GetHwnd(),
			&swapChainDesc,
			nullptr,
			nullptr,
			&swapChain1
		)
	);

	// This sample does not support full screen transitions.
	CHECK_D3D_RESULT (
		dxgiFactory->MakeWindowAssociation(
			Win32Application::GetHwnd(),
			DXGI_MWA_NO_ALT_ENTER
		)
	);

	ComPtr<IDXGISwapChain2> swapChain2;
	CHECK_D3D_RESULT (
		swapChain1.As(&swapChain2)
	);
	swapChain2->SetMaximumFrameLatency(numSwapChainBuffers);

	// Get the frame latency waitable objects.
	*frameLatencyWaitableObject = swapChain2->GetFrameLatencyWaitableObject();

	CHECK_D3D_RESULT (
		// Acquire the IDXGISwapChain3 interface.
		swapChain1.As(&swapChain)
	);
}

//---------------------------------------------------------------------------------------
void waitForFrameFence (
	_In_ ID3D12Fence * fence,
	_In_ uint64 completionValue,
	_In_ HANDLE fenceEvent
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
	std::wstring name
) :
	m_frameIndex(0),
	m_windowWidth(windowWidth),
	m_windowHeight(windowHeight),
	m_title(name),
	m_useWarpDevice(false),
	m_fenceValue{} // default to all zeros.
{
	// Default viewport to size of full window.
	m_viewport.Width = float(windowWidth);
	m_viewport.Height = float(windowHeight);
	m_viewport.MaxDepth = 1.0f;

	// Default to rendering to entire viewport.
	m_scissorRect.right = long(windowWidth);
	m_scissorRect.bottom = long(windowHeight);

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
}

//---------------------------------------------------------------------------------------
static void createCommandQueue (
	_In_ ID3D12Device * device,
	_In_ D3D12_COMMAND_LIST_TYPE queueType,
	_Out_ ComPtr<ID3D12CommandQueue> & commandQueue
) {
	// Describe and create the direct command queue.
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = queueType;

	CHECK_D3D_RESULT (
		device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue))
	);
}

//---------------------------------------------------------------------------------------
D3D12DemoBase::~D3D12DemoBase()
{

}

//---------------------------------------------------------------------------------------
void D3D12DemoBase::initializeDemo()
{
#if defined(_DEBUG)
	//-- Enable the D3D12 debug layer:
	ComPtr<ID3D12Debug> debugController;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
		debugController->EnableDebugLayer();
	}
#endif

	CHECK_D3D_RESULT (
		::CreateDXGIFactory1(IID_PPV_ARGS(&m_dxgiFactory))
	);

	// Create the device.
	::createDevice(m_dxgiFactory.Get(), m_useWarpDevice, m_device);
	NAME_D3D12_OBJECT(m_device);

	// Create a direct D3D12CommandQueue.
	::createCommandQueue(m_device.Get(), D3D12_COMMAND_LIST_TYPE_DIRECT, m_directCmdQueue);
	NAME_D3D12_OBJECT(m_directCmdQueue);

	::createSwapChain (
		m_dxgiFactory.Get(),
		m_directCmdQueue.Get(),
		m_windowWidth,
		m_windowHeight,
		NUM_BUFFERED_FRAMES,
		m_swapChain,
		&m_frameLatencyWaitableObject
	);

	// Set the current frame index to correspond with the current back buffer index.
	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();


	// Create synchronization primitive.
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
	m_currentFenceValue = 1;


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

	//-- Create a RTV for each swapChain buffer.
	{
		// Create a RTV handle that points to the RTV descriptor heap.
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvDescHeapHandle (
			m_rtvDescHeap->GetCPUDescriptorHandleForHeapStart()
		);

		// Specify a RTV with sRGB format to support gamma correction.
		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

		const uint rtvDescriptorSize =
			m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		// Create a render target view for each frame.
		for (uint n(0); n < NUM_BUFFERED_FRAMES; ++n) {
			CHECK_D3D_RESULT (
				m_swapChain->GetBuffer(n, IID_PPV_ARGS(&m_renderTarget[n]))
			);

			// Create RTV and store it at the heap location pointed to by descriptor heap handle.
			m_device->CreateRenderTargetView(m_renderTarget[n].Get(), &rtvDesc, rtvDescHeapHandle);

			// Increment descriptor heap handle so it points to the next RTV in the heap.
			rtvDescHeapHandle.Offset(1, rtvDescriptorSize);
		}
		NAME_D3D12_OBJECT_ARRAY(m_renderTarget, NUM_BUFFERED_FRAMES);
	}
}

//---------------------------------------------------------------------------------------
void D3D12DemoBase::buildNextFrame()
{
	// Wait until GPU has processed the previous indexed frame before building new one.
	::waitForFrameFence (
		m_frameFence[m_frameIndex].Get(),
		m_fenceValue[m_frameIndex],
		m_frameFenceEvent[m_frameIndex]
	);

	update();

	if (m_vsyncEnabled) {
		// Wait until swap chain has finished presenting all queued frames before building
		// command lists and rendering next frame.  This will reduce latency for the next
		// rendered frame.
		WaitForSingleObject(m_frameLatencyWaitableObject, INFINITE);
	}

	render();
}

//---------------------------------------------------------------------------------------
void D3D12DemoBase::presentNextFrame()
{
	if (m_vsyncEnabled) {
		present();

	} else if (swapChainWaitableObjectIsSignaled()) {
		// Swap-chain is available to queue up another Present, so do that now.
		present();
	}
	else { 
		// Start over and rebuild the frame again, rendering to the same indexed back buffer.
		m_directCmdQueue->Signal(m_frameFence[m_frameIndex].Get(), m_currentFenceValue);
		m_fenceValue[m_frameIndex] = m_currentFenceValue;
		++m_currentFenceValue;
	}
}

//---------------------------------------------------------------------------------------
// Present to contents of the back-buffer, signal the frame-fence, then increment the
// frame index so the next frame can be processed.
void D3D12DemoBase::present()
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
__forceinline bool D3D12DemoBase::swapChainWaitableObjectIsSignaled()
{
	return WAIT_OBJECT_0 == WaitForSingleObjectEx(m_frameLatencyWaitableObject, 0, true);
}

//---------------------------------------------------------------------------------------
void D3D12DemoBase::waitForGpuCompletion(
	ID3D12CommandQueue * commandQueue
) {
	CHECK_D3D_RESULT (
		commandQueue->Signal(m_frameFence[m_frameIndex].Get(), m_currentFenceValue)
	);
	m_fenceValue[m_frameIndex] = m_currentFenceValue;
	++m_currentFenceValue;

	::waitForFrameFence(m_frameFence[m_frameIndex].Get(),
		m_fenceValue[m_frameIndex], m_frameFenceEvent[m_frameIndex]);
}

//---------------------------------------------------------------------------------------
void D3D12DemoBase::cleanupDemo()
{
	// Ensure that the GPU is no longer referencing resources that are about to be
	// cleaned up by the destructor.

	m_directCmdQueue->Signal(m_frameFence[m_frameIndex].Get(), m_currentFenceValue);
	m_fenceValue[m_frameIndex] = m_currentFenceValue;
	++m_currentFenceValue;

	// Wait for command queue to finish processing all buffered frames
	for (int i = 0; i < NUM_BUFFERED_FRAMES; ++i) {
		waitForFrameFence(m_frameFence[i].Get(), m_fenceValue[i], m_frameFenceEvent[i]);
	}

	// Clean up event handles
	for (auto event : m_frameFenceEvent) {
		CloseHandle(event);
	}
}

//---------------------------------------------------------------------------------------
void D3D12DemoBase::onKeyDown(uint8 key)
{
	// Empty, to be overridden by derived class
}

//---------------------------------------------------------------------------------------
void D3D12DemoBase::onKeyUp(uint8 key)
{

	// Empty, to be overridden by derived class
}

//---------------------------------------------------------------------------------------
uint D3D12DemoBase::getWindowWidth() const {
	return m_windowWidth;
}


//---------------------------------------------------------------------------------------
uint D3D12DemoBase::getWindowHeight() const {
	return m_windowHeight;
}


//---------------------------------------------------------------------------------------
const WCHAR * D3D12DemoBase::getWindowTitle() const {
	return m_title.c_str();
}


//---------------------------------------------------------------------------------------
std::wstring D3D12DemoBase::getAssetPath(LPCWSTR assetName)
{
    assert(assetName);

    // Compiled shader code .cso files should be in the current working directory,
    // where as all other assets (e.g. textures, meshes, etc.) should be located in
    // the shared asset path.
    const wchar_t * stringMatch = wcsstr(assetName, L".cso");
    if (stringMatch) {
        return m_workingDirPath + assetName;
    }

    // .obj files reside in Assets\Meshes\ folder.
    stringMatch = wcsstr(assetName, L".obj");
    if (stringMatch) {
        return m_sharedAssetPath + L"Meshes\\" + assetName;
    }

    return m_sharedAssetPath + assetName;

}


//---------------------------------------------------------------------------------------
std::string D3D12DemoBase::getAssetPath(const char * assetName)
{
    const int BUFFER_LENGTH = 512;

    wchar_t wcsBuffer[BUFFER_LENGTH]; // wide character string buffer.
    size_t result = mbstowcs(wcsBuffer, assetName, BUFFER_LENGTH);
    if (result == BUFFER_LENGTH) {
        // assetName path is too long to fit in buffer.
        throw;
    }
    std::wstring assetPath = this->getAssetPath(wcsBuffer);

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
void D3D12DemoBase::setCustomWindowText (
	_In_ LPCWSTR text
) {
	std::wstring windowText = m_title + L": " + text;
	SetWindowText(Win32Application::GetHwnd(), windowText.c_str());
}
