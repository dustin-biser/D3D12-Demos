#include "pch.h"
using namespace DirectX;

#include "InstanceRendering.hpp"
using Microsoft::WRL::ComPtr;

#include "MeshLoader.hpp"

#include <vector>
#include <iostream>
using namespace std;



//---------------------------------------------------------------------------------------
// Static Function Declarations
//---------------------------------------------------------------------------------------
static void CreateCommandQueue (
	_In_ ID3D12Device * device,
	_Out_ ComPtr<ID3D12CommandQueue> & commandQueue
);

static void CreateDevice (
    _In_ IDXGIAdapter1 * hardwareAdapter,
	_Out_ ComPtr<ID3D12Device> & device
);

static void CreatePipelineState (
	_In_ ID3D12Device * device,
	_In_ ID3D12RootSignature * rootSignature,
	_In_ ID3DBlob * vertexShaderBlob,
	_In_ ID3DBlob * pixelShaderBlob,
	_Out_ ComPtr<ID3D12PipelineState> & pipelineState
);

static void CreateRenderTargetView(
	_In_ ID3D12Device * device,
	_In_ IDXGISwapChain * swapChain,
	_In_ uint numBufferedFrames,
	_Out_ ComPtr<ID3D12DescriptorHeap> & rtvHeap,
	_Out_ uint & rtvDescriptorSize,
	_Out_ ComPtr<ID3D12Resource> * renderTargets
);

static void CreateSwapChain (
	_In_ IDXGIFactory4 * dxgiFactory,
	_In_ ID3D12CommandQueue * commandQueue,
	_In_ uint framebufferWidth,
	_In_ uint framebufferHeight,
	_In_ uint numBufferedFrames,
	_Out_ ComPtr<IDXGISwapChain3> & swapChain,
	_Out_ HANDLE & frameLatencyWaitableObject
);

static void OutputMemoryBudgets (
    _In_ ID3D12Device * device
);

static void WaitForFrameFence (
	_In_ ID3D12Fence * fence,
	_In_ uint64 completionValue,
	_In_ HANDLE fenceEvent
);


//---------------------------------------------------------------------------------------
InstanceRendering::InstanceRendering (
    uint width, 
    uint height,
    std::wstring name
)   
    :   D3D12DemoBase(width, height, name),
        m_frameIndex(0),
		m_viewport{},       // default to all zeros.
		m_scissorRect{},    // default to all zeros.
		m_fenceValue{}		// default to all zeros.
{
	m_viewport.Width = float(width);
	m_viewport.Height = float(height);
	m_viewport.MaxDepth = 1.0f;

	m_scissorRect.right = long(width);
	m_scissorRect.bottom = long(height);
}


//---------------------------------------------------------------------------------------
void InstanceRendering::InitializeDemo()
{
	LoadPipelineDependencies();

	LoadAssets();

    PopulateCommandList();

	OutputMemoryBudgets(m_device.Get());
}


//---------------------------------------------------------------------------------------
// Loads the rendering pipeline dependencies such as device, command queue, swap chain,
// render target views and command allocator.
void InstanceRendering::LoadPipelineDependencies()
{
#if defined(_DEBUG)
	//-- Enable the D3D12 debug layer:
    ComPtr<ID3D12Debug> debugController;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
    {
        debugController->EnableDebugLayer();
    }
#endif

	ComPtr<IDXGIFactory4> dxgiFactory;
	CHECK_DX_RESULT (
        CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory))
    );

	ComPtr<IDXGIAdapter1> hardwareAdapter;
	GetHardwareAdapter(dxgiFactory.Get(), &hardwareAdapter);

    // Create the device.
    CreateDevice(hardwareAdapter.Get(), m_device);
	NAME_D3D12_OBJECT(m_device);

    // Create the direct command queue.
    CreateCommandQueue (
        m_device.Get(),
        m_commandQueue
    ); NAME_D3D12_OBJECT(m_commandQueue);

    // Create the swap chain.
    CreateSwapChain (
        dxgiFactory.Get(),
        m_commandQueue.Get(),
        m_windowWidth,
        m_windowHeight,
		NUM_BUFFERED_FRAMES,
        m_swapChain,
		m_frameLatencyWaitableObject
    );
    
    // Set the frame index so it corresponds with the current back buffer index.
	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

	//-- Create descriptor heaps:
	{
		// Describe and create the RTV Descriptor Heap.
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDescriptor = {};
		rtvHeapDescriptor.NumDescriptors = NUM_BUFFERED_FRAMES;
		rtvHeapDescriptor.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDescriptor.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		CHECK_DX_RESULT (
			m_device->CreateDescriptorHeap(&rtvHeapDescriptor, IID_PPV_ARGS(&m_rtvDescHeap))
		);

		// Describe and create the CBV Descriptor Heap.
		D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc = {};
		cbvHeapDesc.NumDescriptors = 2 * NUM_BUFFERED_FRAMES;
		cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		CHECK_DX_RESULT (
			m_device->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&m_cbvDescHeap))
		);
	}

	//-- Create a RTV for each swapChain buffer:
	{
		// Create a RTV handle that points into the RTV descriptor heap.
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle (
			m_rtvDescHeap->GetCPUDescriptorHandleForHeapStart()
		);

		// Specify a RTV with sRGB format to support gamma correction.
		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

		uint rtvDescriptorSize = 
			m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		// Create a render target view for each frame.
		for (uint n(0); n < NUM_BUFFERED_FRAMES; ++n) {
			CHECK_DX_RESULT (
				m_swapChain->GetBuffer(n, IID_PPV_ARGS(&m_renderTarget[n]))
			);

			// Create a RTV and store it at the heap location pointed to by rtvHandle.
			m_device->CreateRenderTargetView(m_renderTarget[n].Get(), &rtvDesc, rtvHandle);

			// Increment rtvHandle so it points to the next RTV descriptor in the heap.
			rtvHandle.Offset(1, rtvDescriptorSize);
		}
		NAME_D3D12_OBJECT_ARRAY(m_renderTarget, NUM_BUFFERED_FRAMES);
	}

    //-- Create command allocator for managing command list memory:
	for(int i(0); i < NUM_BUFFERED_FRAMES; ++i) {
		CHECK_DX_RESULT(
			m_device->CreateCommandAllocator(
				D3D12_COMMAND_LIST_TYPE_DIRECT,
				IID_PPV_ARGS(&m_cmdAllocator[i])
			)
		);
	}
	NAME_D3D12_OBJECT_ARRAY(m_cmdAllocator, NUM_BUFFERED_FRAMES);


    //-- Create the draw command lists which will hold our rendering commands:
	{
		// Create one command list for each swap chain buffer.
		for (uint i(0); i < NUM_BUFFERED_FRAMES; ++i) {
			CHECK_DX_RESULT(
				m_device->CreateCommandList(
					0,
					D3D12_COMMAND_LIST_TYPE_DIRECT,
					m_cmdAllocator[i].Get(),
					nullptr, // Will set pipeline state later before drawing
					IID_PPV_ARGS(&m_drawCmdList[i])
				)
			);
			// Stop recording, will reset this later before issuing drawing commands.
			m_drawCmdList[i]->Close();
		}
		NAME_D3D12_OBJECT_ARRAY(m_drawCmdList, NUM_BUFFERED_FRAMES);
	}

    // Create frame synchronization objects.
	for (int i(0); i < NUM_BUFFERED_FRAMES; ++i) {
		CHECK_DX_RESULT(
			m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_frameFence[i]))
		);
		m_fenceValue[i] = 0;

		// Create an event handle to use for frame synchronization.
		m_frameFenceEvent[i] = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (m_frameFenceEvent[i] == nullptr) {
			CHECK_DX_RESULT (
				HRESULT_FROM_WIN32(GetLastError())
			);
		}
	}
	m_currentFenceValue = 1;  
}


//---------------------------------------------------------------------------------------
static void CreateDevice (
    _In_ IDXGIAdapter1 * hardwareAdapter,
	_Out_ ComPtr<ID3D12Device> & device
) {
	// Display hardware adapter name.
	DXGI_ADAPTER_DESC1 adapterDesc = {};
	hardwareAdapter->GetDesc1(&adapterDesc);
	std::wcout << "Adapter: " << adapterDesc.Description << std::endl;

	CHECK_DX_RESULT (
		D3D12CreateDevice (
			hardwareAdapter,
			D3D_FEATURE_LEVEL_11_0,
			IID_PPV_ARGS(&device)
		)
	);
}

//---------------------------------------------------------------------------------------
static void OutputMemoryBudgets (
    _In_ ID3D12Device * device
) {
    assert(device);

    DXGI_QUERY_VIDEO_MEMORY_INFO videoMemoryInfo;

    // Query video memory
	QueryVideoMemoryInfo(device, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, videoMemoryInfo);
    cout << "Video Memory:\n"
         << "Budget: " << videoMemoryInfo.Budget << " bytes" << endl
         << "AvailableForReservation: " << videoMemoryInfo.AvailableForReservation << " bytes" << endl
         << "CurrantUsage: " << videoMemoryInfo.CurrentUsage << " bytes" << endl;
    cout << endl;

    // Query system memory
	QueryVideoMemoryInfo(device, DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL, videoMemoryInfo);
    cout << "System Memory:\n"
         << "Budget: " << videoMemoryInfo.Budget << " bytes" << endl
         << "AvailableForReservation: " << videoMemoryInfo.AvailableForReservation << " bytes" << endl
         << "CurrantUsage: " << videoMemoryInfo.CurrentUsage << " bytes" << endl;
    cout << endl;
}

//---------------------------------------------------------------------------------------
static void CreateCommandQueue (
    _In_ ID3D12Device * device,
    _Out_ ComPtr<ID3D12CommandQueue> & commandQueue
) {
	// Describe and create the direct command queue.
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    CHECK_DX_RESULT(
        device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue))
    );
}


//---------------------------------------------------------------------------------------
void CreateSwapChain (
    _In_ IDXGIFactory4 * dxgiFactory,
    _In_ ID3D12CommandQueue * commandQueue,
    _In_ uint framebufferWidth,
    _In_ uint framebufferHeight,
	_In_ uint numBufferedFrames,
    _Out_ ComPtr<IDXGISwapChain3> & swapChain,
	_Out_ HANDLE & frameLatencyWaitableObject
) {

	// Describe and create the swap chain.
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.BufferCount = numBufferedFrames;
	swapChainDesc.Width = framebufferWidth;
	swapChainDesc.Height = framebufferHeight;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;


	ComPtr<IDXGISwapChain1> swapChain1;
	CHECK_DX_RESULT (
        dxgiFactory->CreateSwapChainForHwnd (
            commandQueue, // Swap chain needs the queue so that it can force a flush on it.
            Win32Application::GetHwnd(),
            &swapChainDesc,
            nullptr,
            nullptr,
            &swapChain1
		)
    );

	// This sample does not support full screen transitions.
	CHECK_DX_RESULT (
        dxgiFactory->MakeWindowAssociation (
            Win32Application::GetHwnd(),
            DXGI_MWA_NO_ALT_ENTER
        )
    );

	ComPtr<IDXGISwapChain2> swapChain2;
	CHECK_DX_RESULT(
		swapChain1.As(&swapChain2)
	);
	swapChain2->SetMaximumFrameLatency(numBufferedFrames);

	// Get the frame latency waitable objects.
	frameLatencyWaitableObject = swapChain2->GetFrameLatencyWaitableObject();

	CHECK_DX_RESULT (
        // Acquire the IDXGISwapChain3 interface.  A reference to this interface will be
        // stored in swapChain.
        swapChain1.As(&swapChain)
    );
}


//---------------------------------------------------------------------------------------
static void CreateRenderTargetView (
    _In_ ID3D12Device * device,
    _In_ IDXGISwapChain * swapChain,
	_In_ uint numBufferedFrames,
    _Out_ ComPtr<ID3D12DescriptorHeap> & rtvHeap,
    _Out_ uint & rtvDescriptorSize,
    _Out_ ComPtr<ID3D12Resource> * renderTargets
) {
    // Describe and create a render target view (RTV) descriptor heap which will
    // hold the RTV descriptors.
	{
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
		rtvHeapDesc.NumDescriptors = numBufferedFrames;
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		CHECK_DX_RESULT (
            device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap))
        );

		rtvDescriptorSize =
            device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}


}

//---------------------------------------------------------------------------------------
void InstanceRendering::LoadAssets()
{
	//-- Create root signature:
	{
		// Two root parameters:
		// First parameter is CBV for SceneConstants
		// Second parameter is CBV for PointLight
		CD3DX12_ROOT_PARAMETER rootParameters[2];

		uint b0(0);
		uint space0(0);
		uint space1(1);
		rootParameters[0].InitAsConstantBufferView(b0, space0);
		rootParameters[1].InitAsConstantBufferView(b0, space1);

		// Allow input layout and deny unnecessary access for certain pipeline stages
		D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
			/* Allow */
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
			/* Deny */
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

		CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
		rootSignatureDesc.Init(_countof(rootParameters), rootParameters, 0, nullptr,
			rootSignatureFlags);

		ComPtr<ID3DBlob> signature;
		ComPtr<ID3DBlob> error;

		CHECK_DX_RESULT(
			D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1,
				&signature, &error)
		);

		CHECK_DX_RESULT(
			m_device->CreateRootSignature(0, signature->GetBufferPointer(),
				signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature))
		);
	}

	//-- Load shader byte code:
	ComPtr<ID3DBlob> vertexShaderBlob;
	ComPtr<ID3DBlob> pixelShaderBlob;
	D3DReadFileToBlob(GetAssetPath(L"VertexShader.cso").c_str(), &vertexShaderBlob);
	D3DReadFileToBlob(GetAssetPath(L"PixelShader.cso").c_str(), &pixelShaderBlob);

	// Create the pipeline state object.
	CreatePipelineState(
		m_device.Get(),
		m_rootSignature.Get(),
		vertexShaderBlob.Get(),
		pixelShaderBlob.Get(),
		m_pipelineState
	); NAME_D3D12_OBJECT(m_pipelineState);

	// Create upload buffer to hold graphics resources
	m_uploadBuffer = std::make_shared<ResourceUploadBuffer>(
		m_device.Get(),
		128 * 1024 // 128 KB
	);

	// Cube vertex data.
	m_vertexArray = {
		// Positions             Normals
		// Bottom
		{ -0.5f, -0.5f,  0.5f,   0.0f, -1.0f,  0.0f}, // 0
		{  0.5f, -0.5f,  0.5f,   0.0f, -1.0f,  0.0f}, // 1
		{  0.5f, -0.5f, -0.5f,   0.0f, -1.0f,  0.0f}, // 2
		{ -0.5f, -0.5f, -0.5f,   0.0f, -1.0f,  0.0f}, // 3

		// Top
		{ -0.5f,  0.5f,  0.5f,   0.0f,  1.0f,  0.0f}, // 4
		{  0.5f,  0.5f,  0.5f,   0.0f,  1.0f,  0.0f}, // 5
		{  0.5f,  0.5f, -0.5f,   0.0f,  1.0f,  0.0f}, // 6
		{ -0.5f,  0.5f, -0.5f,   0.0f,  1.0f,  0.0f}, // 7

		// Left 
		{ -0.5f, -0.5f,  0.5f,  -1.0f,  0.0f,  0.0f}, // 8
		{ -0.5f, -0.5f, -0.5f,  -1.0f,  0.0f,  0.0f}, // 9
		{ -0.5f,  0.5f,  0.5f,  -1.0f,  0.0f,  0.0f}, // 10
		{ -0.5f,  0.5f, -0.5f,  -1.0f,  0.0f,  0.0f}, // 11

		// Back 
		{ -0.5f, -0.5f, -0.5f,   0.0f,  0.0f, -1.0f}, // 12
		{  0.5f, -0.5f, -0.5f,   0.0f,  0.0f, -1.0f}, // 13
		{  0.5f,  0.5f, -0.5f,   0.0f,  0.0f, -1.0f}, // 14
		{ -0.5f,  0.5f, -0.5f,   0.0f,  0.0f, -1.0f}, // 15

		// Right 
		{  0.5f, -0.5f,  0.5f,   1.0f,  0.0f,  0.0f}, // 16
		{  0.5f, -0.5f, -0.5f,   1.0f,  0.0f,  0.0f}, // 17
		{  0.5f,  0.5f, -0.5f,   1.0f,  0.0f,  0.0f}, // 18
		{  0.5f,  0.5f,  0.5f,   1.0f,  0.0f,  0.0f}, // 19

		// Front 
		{ -0.5f, -0.5f,  0.5f,   0.0f,  0.0f,  1.0f}, // 20
		{  0.5f, -0.5f,  0.5f,   0.0f,  0.0f,  1.0f}, // 21
		{  0.5f,  0.5f,  0.5f,   0.0f,  0.0f,  1.0f}, // 22
		{ -0.5f,  0.5f,  0.5f,   0.0f,  0.0f,  1.0f}, // 23
	};


	// Upload vertex data and create Vertex Buffer View:
	{
		size_t vertexSize = sizeof(Vertex);
		size_t dataBytes = m_vertexArray.size() * vertexSize;
		m_uploadBuffer->uploadVertexData(
			m_vertexArray.data(),
			dataBytes,
			vertexSize,
			m_vertexBufferView
		);
	}

	// Create Index data
	m_indexArray = {
		// Bottom
		3,1,0, 3,2,1,
		// Top
		7,4,5, 7,5,6,
		// Left
		8,10,9, 10,11,9,
		// Back
		12,15,13, 15,14,13,
		// Right
		16,17,19, 17,18,19,
		// Front
		20,21,23, 21,22,23
	};

	//-- Upload index data and create Index Buffer View:
	{
		size_t indexSize = sizeof(Index);
		size_t dataBytes = m_indexArray.size() * indexSize;

		m_uploadBuffer->uploadIndexData(
			m_indexArray.data(),
			dataBytes,
			indexSize,
			m_indexBufferView
		);
	}

	// Create SceneConstants ConstantBuffer storage within upload heap, duplicating
	// storage space for each buffered frame.
	size_t sceneConstantsSize = sizeof(SceneConstants);
	for (int i(0); i < NUM_BUFFERED_FRAMES; ++i) {
		ZeroMemory(&m_sceneConstData[i], sceneConstantsSize);
		m_uploadBuffer->uploadConstantBufferData(
			reinterpret_cast<const void *>(&m_sceneConstData[i]),
			sceneConstantsSize,
			m_cbvDesc_SceneConstants[i],
			&m_cbv_SceneConstants_dataPtr[i]
		);
	}

	// Create PointLight ConstantBuffer storage within upload heap, duplicating
	// storage space for each buffered frame.
	size_t pointLightSize = sizeof(PointLight);
	for (int i(0); i < NUM_BUFFERED_FRAMES; ++i) {
		ZeroMemory(&m_pointLightConstData[i], pointLightSize);
		m_uploadBuffer->uploadConstantBufferData(
			reinterpret_cast<const void *>(&m_pointLightConstData[i]),
			pointLightSize,
			m_cbvDesc_PointLight[i],
			&m_cbv_PointLight_dataPtr[i]
		);
	}


	//-- Create CBV on the CBV-Heap that references our ConstantBuffer data.
	for (int i(0); i < NUM_BUFFERED_FRAMES; ++i) {
		CD3DX12_CPU_DESCRIPTOR_HANDLE cbvDescHeapHandle(
			m_cbvDescHeap->GetCPUDescriptorHandleForHeapStart()
		);

		m_device->CreateConstantBufferView(&m_cbvDesc_SceneConstants[i], cbvDescHeapHandle);

		// Increment cbvDescHandle to point to next descriptor in cbvHeap.
		uint descriptorIncrementSize =
			m_device->GetDescriptorHandleIncrementSize(
				D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
			);
		cbvDescHeapHandle.Offset(1, descriptorIncrementSize);

		m_device->CreateConstantBufferView(&m_cbvDesc_PointLight[i], cbvDescHeapHandle);
	}


	// Create the depth/stencil buffer
	{
		// create a depth stencil descriptor heap so we can get a pointer to the depth stencil buffer
		D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDescriptor = {};
		dsvHeapDescriptor.NumDescriptors = 1;
		dsvHeapDescriptor.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		dsvHeapDescriptor.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		CHECK_DX_RESULT (
			m_device->CreateDescriptorHeap(&dsvHeapDescriptor, IID_PPV_ARGS(&m_dsvDescHeap))
		);

		D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
		depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;
		depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;

		D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
		depthOptimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
		depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
		depthOptimizedClearValue.DepthStencil.Stencil = 0;

		CHECK_DX_RESULT(
			m_device->CreateCommittedResource (
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
				D3D12_HEAP_FLAG_NONE,
				&CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, m_windowWidth, m_windowHeight,
					1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL),
				D3D12_RESOURCE_STATE_DEPTH_WRITE,
				&depthOptimizedClearValue,
				IID_PPV_ARGS(&m_depthStencilBuffer)
			)
		);
		NAME_D3D12_OBJECT(m_dsvDescHeap);

		m_device->CreateDepthStencilView (
			m_depthStencilBuffer.Get(),
			&depthStencilDesc,
			m_dsvDescHeap->GetCPUDescriptorHandleForHeapStart()
		);
	}
}


//---------------------------------------------------------------------------------------
static void CreateRootSignature (
    _In_ ID3D12Device * device,
    _Out_ ComPtr<ID3D12RootSignature> & rootSignature
) {
}

//---------------------------------------------------------------------------------------
static void CreatePipelineState (
    _In_ ID3D12Device * device,
    _In_ ID3D12RootSignature * rootSignature,
    _In_ ID3DBlob * vertexShaderBlob,
    _In_ ID3DBlob * pixelShaderBlob,
    _Out_ ComPtr<ID3D12PipelineState> & pipelineState
) {
    // Define the vertex input layout.
    D3D12_INPUT_ELEMENT_DESC inputElementDescriptor[2];

    // Positions
    inputElementDescriptor[0].SemanticName = "POSITION";
    inputElementDescriptor[0].SemanticIndex = 0;
    inputElementDescriptor[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
    inputElementDescriptor[0].InputSlot = 0;
    inputElementDescriptor[0].AlignedByteOffset = 0;
    inputElementDescriptor[0].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
    inputElementDescriptor[0].InstanceDataStepRate = 0;

    // Normals
    inputElementDescriptor[1].SemanticName = "NORMAL";
    inputElementDescriptor[1].SemanticIndex = 0;
    inputElementDescriptor[1].Format = DXGI_FORMAT_R32G32B32_FLOAT;
    inputElementDescriptor[1].InputSlot = 0;
    inputElementDescriptor[1].AlignedByteOffset = sizeof(float) * 3;
    inputElementDescriptor[1].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
    inputElementDescriptor[1].InstanceDataStepRate = 0;

    // Describe the rasterizer state
    CD3DX12_RASTERIZER_DESC rasterizerState(D3D12_DEFAULT);
    rasterizerState.FrontCounterClockwise = TRUE;
	rasterizerState.CullMode = D3D12_CULL_MODE_BACK;
	rasterizerState.DepthClipEnable = FALSE;

	D3D12_INPUT_LAYOUT_DESC
		inputLayoutDesc = { inputElementDescriptor, _countof(inputElementDescriptor) };

    // Describe and create the graphics pipeline state object (PSO).
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = inputLayoutDesc;
    psoDesc.pRootSignature = rootSignature;
    psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShaderBlob);
    psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShaderBlob);
    psoDesc.RasterizerState = rasterizerState;
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

	DXGI_SAMPLE_DESC sampleDesc = {};
	sampleDesc.Count = 1;
	psoDesc.SampleDesc = sampleDesc;

    // Create the Pipeline State Object.
    CHECK_DX_RESULT (
        device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState))
     );
}

//---------------------------------------------------------------------------------------
void InstanceRendering::UpdateConstantBuffers()
{
	//-- Create and Upload SceneContants Data:
	{
		static float rotationAngle(0.0f);
		const float rotationDelta(1.2f);
		rotationAngle += rotationDelta;

		XMVECTOR axis = XMVectorSet(0.5f, 1.0f, 0.5f, 0.0f);
		XMMATRIX rotate = XMMatrixRotationAxis(axis, XMConvertToRadians(rotationAngle));

		XMMATRIX modelMatrix = XMMatrixMultiply(rotate, XMMatrixTranslation(0.0f, 0.0f, -5.0f));

		XMMATRIX viewMatrix = XMMatrixLookAtRH(
			XMVECTOR{ 0.0f, 0.0f, 0.0f, 1.0f },
			XMVECTOR{ 0.0f, 0.0f, -100.0f, 1.0f },
			XMVECTOR{ 0.0f, 1.0f, 0.0f, 0.0f }
		);

		XMMATRIX modelViewMatrix = XMMatrixMultiply(modelMatrix, viewMatrix);

		// Construct perspective projection matrix:
		XMMATRIX projectMatrix;
		{
			// Undefine Windows' dumb defines
			#ifdef near
			#undef near
			#endif
			#ifdef far
			#undef far
			#endif

			float near(0.1f);
			float far(200.0f);
			float inv_n_minus_f = 1.0f / (near - far);
			float fovy(XMConvertToRadians(30.0f));

			float m11 = 1.0f / tan(fovy);
			float inv_aspect = static_cast<float>(m_windowHeight) / m_windowWidth;
			float m00 = m11 * inv_aspect;
			projectMatrix = XMMatrixSet(
				m00, 0.0f, 0.0f, 0.0f,
				0.0f, m11, 0.0f, 0.0f,
				0.0f, 0.0f, far*inv_n_minus_f, -1.0f,
				0.0f, 0.0f, near*far*inv_n_minus_f, 0.0f
			);
		}

		XMMATRIX MVPMatrix = XMMatrixMultiply(modelMatrix, viewMatrix);
		MVPMatrix = XMMatrixMultiply(MVPMatrix, projectMatrix);

		XMMATRIX invMatrix = XMMatrixInverse(nullptr, modelViewMatrix);
		XMMATRIX normalMatrix = XMMatrixTranspose(invMatrix);

		XMStoreFloat4x4(&m_sceneConstData[m_frameIndex].modelViewMatrix, XMMatrixTranspose(modelViewMatrix));
		XMStoreFloat4x4(&m_sceneConstData[m_frameIndex].MVPMatrix, XMMatrixTranspose(MVPMatrix));
		XMStoreFloat4x4(&m_sceneConstData[m_frameIndex].normalMatrix, XMMatrixTranspose(normalMatrix));


		XMVECTOR lightPosition{ -5.0f, 5.0f,  5.0f, 1.0f };

		// Transform lightPosition into View Space
		lightPosition = XMVector4Transform(lightPosition, viewMatrix);
		XMStoreFloat4(&m_pointLightConstData[m_frameIndex].position_eyeSpace, lightPosition);

		// White light
		m_pointLightConstData[m_frameIndex].color = XMFLOAT4{ 1.0f, 1.0f, 1.0f, 1.0f };


		// Update SceneConstants ConstantBuffer data:
		memcpy (
			m_cbv_SceneConstants_dataPtr[m_frameIndex], 
			&m_sceneConstData[m_frameIndex],
			sizeof(SceneConstants)
		);

		// Update PointLight ConstantBuffer data:
		memcpy (
			m_cbv_PointLight_dataPtr[m_frameIndex], 
			&m_pointLightConstData[m_frameIndex],
			sizeof(PointLight)
		);
	}
}


//---------------------------------------------------------------------------------------
// Perform per-frame logic + rendering
void InstanceRendering::Update()
{
	WaitForFrameFence (
		m_frameFence[m_frameIndex].Get(),
		m_fenceValue[m_frameIndex],
		m_frameFenceEvent[m_frameIndex]
	);

	UpdateConstantBuffers();

	if(m_vsyncEnabled) {
		// Wait until swap chain has finished presenting all queued frames
		WaitForSingleObject(m_frameLatencyWaitableObject, INFINITE);
		Render();
		Present();
	} else if (SwapChainWaitableObjectIsSignaled()) {
		Render();
		Present();
	}
}

//---------------------------------------------------------------------------------------
bool InstanceRendering::SwapChainWaitableObjectIsSignaled()
{
	return WAIT_OBJECT_0 == WaitForSingleObjectEx(m_frameLatencyWaitableObject, 0, true);
}


//---------------------------------------------------------------------------------------
// Render the scene.
void InstanceRendering::Render()
{
	PopulateCommandList();

	// Execute command list for the current frame index
	ID3D12CommandList * commandLists[] = { m_drawCmdList[m_frameIndex].Get() };
	m_commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);
}

//---------------------------------------------------------------------------------------
// Present to contents of the back-buffer, signal the frame-fence, then increment the
// frame index so the next frame can be processed.
void InstanceRendering::Present()
{
	uint syncInterval = m_vsyncEnabled ? 1 : 0;
	CHECK_DX_RESULT(
		m_swapChain->Present(syncInterval, 0)
	);

	m_commandQueue->Signal(m_frameFence[m_frameIndex].Get(), m_currentFenceValue);
	m_fenceValue[m_frameIndex] = m_currentFenceValue;
	++m_currentFenceValue;

	m_frameIndex = (m_frameIndex + 1) % NUM_BUFFERED_FRAMES;
}

//---------------------------------------------------------------------------------------
void InstanceRendering::CleanupDemo()
{
	m_commandQueue->Signal(m_frameFence[m_frameIndex].Get(), m_currentFenceValue);
	m_fenceValue[m_frameIndex] = m_currentFenceValue;
	++m_currentFenceValue;

	// Wait for command queue to finish processing all buffered frames
	for (int i = 0; i < NUM_BUFFERED_FRAMES; ++i) {
		WaitForFrameFence(m_frameFence[i].Get(), m_fenceValue[i], m_frameFenceEvent[i]);
	}

	// Clean up event handles
	for (auto event : m_frameFenceEvent) {
		CloseHandle(event);
	}
}

//---------------------------------------------------------------------------------------

void InstanceRendering::PopulateCommandList()
{
	auto * cmdAllocator = m_cmdAllocator[m_frameIndex].Get();
	auto & drawCmdList = m_drawCmdList[m_frameIndex];

    CHECK_DX_RESULT (
        cmdAllocator->Reset()
    );

	CHECK_DX_RESULT (
		drawCmdList->Reset(cmdAllocator, m_pipelineState.Get())
	 );

	drawCmdList->SetPipelineState(m_pipelineState.Get());

	drawCmdList->SetGraphicsRootSignature(m_rootSignature.Get());

	ID3D12DescriptorHeap * ppHeaps[] = { m_cbvDescHeap.Get() };
	drawCmdList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
	drawCmdList->SetGraphicsRootConstantBufferView (
		0, m_cbvDesc_SceneConstants[m_frameIndex].BufferLocation
	);
	drawCmdList->SetGraphicsRootConstantBufferView (
		1, m_cbvDesc_PointLight[m_frameIndex].BufferLocation
	);

	drawCmdList->RSSetViewports(1, &m_viewport);
	drawCmdList->RSSetScissorRects(1, &m_scissorRect);

	// Indicate that the back buffer will be used as a render target.
	drawCmdList->ResourceBarrier (1,
		&CD3DX12_RESOURCE_BARRIER::Transition (
			m_renderTarget[m_frameIndex].Get(),
			D3D12_RESOURCE_STATE_PRESENT,
			D3D12_RESOURCE_STATE_RENDER_TARGET
		)
	);

	uint rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle (
		m_rtvDescHeap->GetCPUDescriptorHandleForHeapStart(),
		m_frameIndex, 
		rtvDescriptorSize
	);
	// Get a handle to the depth/stencil buffer
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_dsvDescHeap->GetCPUDescriptorHandleForHeapStart());
	drawCmdList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

	// Record commands.
	const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
	drawCmdList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

	// clear the depth/stencil buffer
	drawCmdList->ClearDepthStencilView (
		m_dsvDescHeap->GetCPUDescriptorHandleForHeapStart(),
		D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr
	);

	drawCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	drawCmdList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
	drawCmdList->IASetIndexBuffer(&m_indexBufferView);
	
	UINT numIndices = static_cast<UINT>(m_indexArray.size());
	drawCmdList->DrawIndexedInstanced(numIndices, 1, 0, 0, 0);

	// Indicate that the back buffer will now be used to present.
	drawCmdList->ResourceBarrier (1,
		&CD3DX12_RESOURCE_BARRIER::Transition (
			m_renderTarget[m_frameIndex].Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PRESENT
		)
	);

	CHECK_DX_RESULT(drawCmdList->Close());
}

//---------------------------------------------------------------------------------------
static void WaitForFrameFence (
	_In_ ID3D12Fence * fence,
	_In_ uint64 completionValue,
	_In_ HANDLE fenceEvent
) {
	if (fence->GetCompletedValue() < completionValue) {
		CHECK_DX_RESULT (
			fence->SetEventOnCompletion(completionValue, fenceEvent)
		);
		WaitForSingleObject(fenceEvent, INFINITE);
	}
}
