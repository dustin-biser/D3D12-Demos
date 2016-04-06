//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

#include "pch.h"
#include "IndexRendering.h"

#include <vector>
#include <iostream>
using namespace std;


//---------------------------------------------------------------------------------------
IndexRendering::IndexRendering (
    uint width, 
    uint height,
    std::wstring name
)   
    :   DXSample(width, height, name),
        m_frameIndex(0),
        m_viewport(),
        m_scissorRect(),
        m_rtvDescriptorSize(0)
{
	m_viewport.Width = float(width);
	m_viewport.Height = float(height);
	m_viewport.MaxDepth = 1.0f;

	m_scissorRect.right = long(width);
	m_scissorRect.bottom = long(height);
}

//---------------------------------------------------------------------------------------
void IndexRendering::OnInit()
{
	LoadRenderPipelineDependencies();
	LoadAssets();
    PopulateCommandList();
}



//---------------------------------------------------------------------------------------
// Loads the rendering pipeline dependencies such as
// device, command queue, swap chain, and render target views.
void IndexRendering::LoadRenderPipelineDependencies()
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
	ThrowIfFailed (
        CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory))
    );


    // Create the device.
    this->CreateDevice(dxgiFactory.Get());

    // Create the direct command queue.
    this->CreateCommandQueue(m_device.Get());

    // Create the swap chain.
    this->CreateSwapChain(dxgiFactory.Get(), m_commandQueue.Get());

    // Create RTVs, one for each SwapChain buffer.
    this->CreateRenderTargetView(m_device.Get (), m_swapChain.Get ());


    // Create command allocator for managing command list memory
	ThrowIfFailed (
        m_device->CreateCommandAllocator (
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            IID_PPV_ARGS(&m_commandAllocator)
        )
    );
    NAME_D3D12_OBJECT(m_commandAllocator);
}

//---------------------------------------------------------------------------------------
void IndexRendering::CreateDevice (
    IDXGIFactory4 * dxgiFactory
) {
	if (m_useWarpDevice)
	{
		ComPtr<IDXGIAdapter> warpAdapter;
		ThrowIfFailed (
            dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter))
        );

        // Display warp adapter name.
        DXGI_ADAPTER_DESC adapterDesc = {};
        warpAdapter->GetDesc(&adapterDesc);
        std::wcout << "Adapter: " << adapterDesc.Description << std::endl;

		ThrowIfFailed (
            D3D12CreateDevice (
                warpAdapter.Get(),
                D3D_FEATURE_LEVEL_11_0,
                IID_PPV_ARGS(&m_device)
            )
        );
	}
	else
	{
		ComPtr<IDXGIAdapter1> hardwareAdapter;
		GetHardwareAdapter(dxgiFactory, &hardwareAdapter);

        // Display hardware adapter name.
        DXGI_ADAPTER_DESC1 adapterDesc = {};
        hardwareAdapter->GetDesc1(&adapterDesc);
        std::wcout << "Adapter: " << adapterDesc.Description << std::endl;

		ThrowIfFailed (
            D3D12CreateDevice (
                hardwareAdapter.Get(),
			    D3D_FEATURE_LEVEL_11_0,
			    IID_PPV_ARGS(&m_device)
            )
        );
	}
    NAME_D3D12_OBJECT(m_device);
}

//---------------------------------------------------------------------------------------
void IndexRendering::CreateCommandQueue (
    ID3D12Device * device
) {
	// Describe and create the direct command queue.
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    ThrowIfFailed(
        device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue))
    );
    NAME_D3D12_OBJECT(m_commandQueue);
}


//---------------------------------------------------------------------------------------
void IndexRendering::CreateSwapChain (
    IDXGIFactory4 * dxgiFactory,
    ID3D12CommandQueue * commandQueue
) {

	// Describe and create the swap chain.
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.BufferCount = FrameCount;
	swapChainDesc.Width = m_width;
	swapChainDesc.Height = m_height;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.SampleDesc.Count = 1;

	ComPtr<IDXGISwapChain1> swapChain;
	ThrowIfFailed (
        dxgiFactory->CreateSwapChainForHwnd (
            commandQueue,  // Swap chain needs the queue so that it can force a flush on it.
            Win32Application::GetHwnd(),
            &swapChainDesc,
            nullptr,
            nullptr,
            &swapChain
		)
    );

	// This sample does not support full screen transitions.
	ThrowIfFailed (
        dxgiFactory->MakeWindowAssociation(Win32Application::GetHwnd(), DXGI_MWA_NO_ALT_ENTER)
    );

	ThrowIfFailed (
        // Acquire the IDXGISwapChain3 interface.  A reference to this interface will be
        // stored in m_swapChain.
        swapChain.As(&m_swapChain)
    );

    // Set the current frame index to correspond with the current back buffer index.
	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
}


//---------------------------------------------------------------------------------------
void IndexRendering::CreateRenderTargetView (
    ID3D12Device * device,
    IDXGISwapChain * swapChain
) {
	// Create RTV descriptor heap.
	{
		// Describe and create a render target view (RTV) descriptor heap.
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
		rtvHeapDesc.NumDescriptors = FrameCount;
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		ThrowIfFailed (
            m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap))
        );

		m_rtvDescriptorSize =
            m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}

	// Create a RTV for each swapChain buffer.
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle (
            m_rtvHeap->GetCPUDescriptorHandleForHeapStart()
        );

        // Create a RTV with sRGB format so output image is properly gamma corrected.
        D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
        rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

		// Create a RenderTargetView for each frame.
		for (uint n(0); n < FrameCount; ++n)
		{
			ThrowIfFailed (
                m_swapChain->GetBuffer(n, IID_PPV_ARGS(&m_renderTargets[n]))
            );
			m_device->CreateRenderTargetView(m_renderTargets[n].Get(), &rtvDesc, rtvHandle);
			rtvHandle.Offset(1, m_rtvDescriptorSize);
		}
	}

}

//---------------------------------------------------------------------------------------
// Load the sample assets.
void IndexRendering::LoadAssets()
{
	// Create an empty root signature.
    this->CreateRootSignature(m_device.Get());

    // Compile and load shaders.
    ComPtr<ID3DBlob> vertexShader;
    ComPtr<ID3DBlob> pixelShader;
    this->LoadShaders(vertexShader, pixelShader);

	// Create the pipeline state object.
    this->CreatePipelineState(m_device.Get(), vertexShader.Get(), pixelShader.Get());

    // Create the draw command lists which will hold our rendering commands.
    this->CreateDrawCommandLists (
        m_device.Get(),
        m_commandAllocator.Get(),
        m_pipelineState.Get()
    );

    this->CommitVertexDataToGPU();
}

//---------------------------------------------------------------------------------------
void IndexRendering::CreateRootSignature (
    ID3D12Device * device
) {
    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Init (
        0, nullptr, 0, nullptr,
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
    );

    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;

    ThrowIfFailed (
        D3D12SerializeRootSignature (
            &rootSignatureDesc,
            D3D_ROOT_SIGNATURE_VERSION_1, 
            &signature, 
            &error
        )
    );

    ThrowIfFailed (
        m_device->CreateRootSignature (
            0,
            signature->GetBufferPointer(),
            signature->GetBufferSize(),
            IID_PPV_ARGS(&m_rootSignature))
    );
}


//---------------------------------------------------------------------------------------
void IndexRendering::LoadShaders (
    _Inout_ ComPtr<ID3DBlob> & vertexShader,
    _Inout_ ComPtr<ID3DBlob> & pixelShader
) {
#if defined(_DEBUG)
        // Enable better shader debugging with the graphics debugging tools.
        uint compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
        uint compileFlags = 0;
#endif

    // Compile vertex shader
    ThrowIfFailed (
        D3DCompileFromFile(
            GetAssetFullPath(L"shaders.hlsl").c_str(),
            nullptr, nullptr, "VSMain", "vs_5_0",
            compileFlags, 0,
            vertexShader.ReleaseAndGetAddressOf(),
            nullptr
        )
    );

    // Compile pixel shader
    ThrowIfFailed (
        D3DCompileFromFile(
            GetAssetFullPath(L"shaders.hlsl").c_str(),
            nullptr, nullptr, "PSMain", "ps_5_0",
            compileFlags, 0,
            pixelShader.ReleaseAndGetAddressOf(),
            nullptr
        )
    );
}

//---------------------------------------------------------------------------------------
void IndexRendering::CreatePipelineState(
    ID3D12Device * device,
    ID3DBlob * vertexShader,
    ID3DBlob * pixelShader
) {
    //-- Define the vertex input layout:
    D3D12_INPUT_ELEMENT_DESC inputElementDescriptor[2];
    inputElementDescriptor[0].SemanticName = "POSITION";
    inputElementDescriptor[0].SemanticIndex = 0;
    inputElementDescriptor[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
    inputElementDescriptor[0].InputSlot = 0;
    inputElementDescriptor[0].AlignedByteOffset = 0;
    inputElementDescriptor[0].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
    inputElementDescriptor[0].InstanceDataStepRate = 0;

    inputElementDescriptor[1].SemanticName = "COLOR";
    inputElementDescriptor[1].SemanticIndex = 0;
    inputElementDescriptor[1].Format = DXGI_FORMAT_R32G32B32_FLOAT;
    inputElementDescriptor[1].InputSlot = 0;
    inputElementDescriptor[1].AlignedByteOffset = sizeof(float) * 3;
    inputElementDescriptor[1].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
    inputElementDescriptor[1].InstanceDataStepRate = 0;

    // Describe the rasterizer state
    CD3DX12_RASTERIZER_DESC rasterizerState(D3D12_DEFAULT);
    rasterizerState.FrontCounterClockwise = TRUE;

    // Describe and create the graphics pipeline state object (PSO).
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { inputElementDescriptor, _countof(inputElementDescriptor) };
    psoDesc.pRootSignature = m_rootSignature.Get();
    psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader);
    psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader);
    psoDesc.RasterizerState = rasterizerState;
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState.DepthEnable = FALSE;
    psoDesc.DepthStencilState.StencilEnable = FALSE;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    psoDesc.SampleDesc.Count = 1;

    // Create the Pipeline State Object.
    ThrowIfFailed (
        device->CreateGraphicsPipelineState (
            &psoDesc,
            IID_PPV_ARGS(&m_pipelineState)
        )
     );
}

//---------------------------------------------------------------------------------------
void IndexRendering::CreateDrawCommandLists (
    ID3D12Device * device,
    ID3D12CommandAllocator * commandAllocator,
    ID3D12PipelineState * pipelineState
) {
	// Create a command lists for each swap chain buffer
    for (int i(0); i < FrameCount; ++i) {
        ThrowIfFailed(
            device->CreateCommandList (
                0,
                D3D12_COMMAND_LIST_TYPE_DIRECT,
                commandAllocator,
                pipelineState,
                IID_PPV_ARGS(&m_drawCommandList[i])
            )
        );
        wchar_t buffer[25];
        wsprintf(buffer, L"m_drawCommandList[%d]", i);
        SetName(m_drawCommandList[i].Get(), buffer);

        // Stop recording, will reset this later before issuing drawing commands.
        m_drawCommandList[i]->Close();
    }
}

//---------------------------------------------------------------------------------------
D3D12_VERTEX_BUFFER_VIEW IndexRendering::UploadVertexDataToDefaultHeap(
    _In_ ID3D12Device * device,
    _In_ ID3D12GraphicsCommandList * copyCommandList,
    _Inout_ ComPtr<ID3D12Resource> & vertexBufferUploadHeap,
    _Inout_ ComPtr<ID3D12Resource> & vertexBuffer
) {
    // Define vertices for a square.
    const Vertex vertexData[] =
    {
        // Positions                              Colors
        { { 0.25f, 0.25f * m_aspectRatio, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
        { { 0.25f, -0.25f * m_aspectRatio, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
        { { -0.25f, -0.25f * m_aspectRatio, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } },
        { { -0.25f, 0.25f * m_aspectRatio, 0.0f }, { 1.0f, 0.0f, 1.0f, 1.0f } }
    };
    const uint vertexBufferSize = sizeof(vertexData);

    // The vertex buffer resource will live in the Default Heap, and will
    // be the copy destination.
    ThrowIfFailed(
        device->CreateCommittedResource (
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize),
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_PPV_ARGS(&vertexBuffer))
    );

    // The Upload Heap will contain the raw vertex data, and will be the copy source.
    ThrowIfFailed(
        device->CreateCommittedResource (
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize),
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&vertexBufferUploadHeap)
        )
    );

    // Copy data to the intermediate Upload Heap and then schedule a copy 
    // from the Upload Heap to the vertex buffer resource.
    {
        D3D12_SUBRESOURCE_DATA vertexDataSubResource = {};
        vertexDataSubResource.pData = vertexData;
        vertexDataSubResource.RowPitch = vertexBufferSize;
        vertexDataSubResource.SlicePitch = vertexDataSubResource.RowPitch;

        UpdateSubresources<1>(
            copyCommandList,
            vertexBuffer.Get(),
            vertexBufferUploadHeap.Get(),
            0, 0, 1,
            &vertexDataSubResource
        );

        // Transition the vertex buffer resource from the Copy Destination State to
        // the Vertex Buffer State so it can be fed to the 3D pipeline for rendering.
        copyCommandList->ResourceBarrier (
            1, &CD3DX12_RESOURCE_BARRIER::Transition (
                vertexBuffer.Get(),
                D3D12_RESOURCE_STATE_COPY_DEST,
                D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER)
        );
    }

    // Initialize the vertex buffer view.
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
    vertexBufferView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
    vertexBufferView.StrideInBytes = sizeof(Vertex);
    vertexBufferView.SizeInBytes = vertexBufferSize;

    return vertexBufferView;
}


//---------------------------------------------------------------------------------------
D3D12_INDEX_BUFFER_VIEW IndexRendering::UploadIndexDataToDefaultHeap(
    _In_ ID3D12Device * device,
    _In_ ID3D12GraphicsCommandList * copyCommandList,
    _Inout_ ComPtr<ID3D12Resource> & indexBufferUploadHeap,
    _Inout_ ComPtr<ID3D12Resource> & indexBuffer
) {
    std::vector<ushort> indices = { 2, 1,0, 2,0,3 };
    m_indexCount = uint(indices.size());
    const uint indexBufferSize = sizeof(ushort) * m_indexCount;

    ThrowIfFailed(
        device->CreateCommittedResource (
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize),
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_PPV_ARGS(&indexBuffer))
    );

    ThrowIfFailed(
        device->CreateCommittedResource (
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize),
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&indexBufferUploadHeap))
    );

    // Copy data to the intermediate upload heap and then schedule a copy 
    // from the upload heap to the index buffer.
    {
        D3D12_SUBRESOURCE_DATA indexDataSubResource = {};
        indexDataSubResource.pData = indices.data();
        indexDataSubResource.RowPitch = indexBufferSize;
        indexDataSubResource.SlicePitch = indexDataSubResource.RowPitch;

        UpdateSubresources<1>(
            copyCommandList,
            indexBuffer.Get(),
            indexBufferUploadHeap.Get(),
            0, 0, 1,
            &indexDataSubResource
        );

        // Transition the index buffer resource from the Copy Destination State to
        // the Index Buffer State so it can be fed to the 3D pipeline for rendering.
        copyCommandList->ResourceBarrier (
            1, &CD3DX12_RESOURCE_BARRIER::Transition (
                indexBuffer.Get(),
                D3D12_RESOURCE_STATE_COPY_DEST,
                D3D12_RESOURCE_STATE_INDEX_BUFFER)
        );
    }

    // Initialize the vertex buffer view.
    D3D12_INDEX_BUFFER_VIEW indexBufferView;
    indexBufferView.BufferLocation = indexBuffer->GetGPUVirtualAddress();
    indexBufferView.Format = DXGI_FORMAT_R16_UINT;
    indexBufferView.SizeInBytes = indexBufferSize;

    return indexBufferView;
}


//---------------------------------------------------------------------------------------
void IndexRendering::CommitVertexDataToGPU()
{
    // Create a separate command list for copying resource data to the GPU.
    ComPtr<ID3D12GraphicsCommandList> copyCommandList;
    ThrowIfFailed (
        m_device->CreateCommandList (
            0,
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            m_commandAllocator.Get(),
            m_pipelineState.Get(),
            IID_PPV_ARGS(&copyCommandList)
         )
    );

    // Upload heaps are only needed when loading data into GPU memory.
    // Note: ComPtr's are CPU objects but this resource needs to stay in scope until
    // the command list that references it has finished executing on the GPU.
    // We will flush the GPU at the end of this method to ensure the resource is not
    // prematurely destroyed.
    ComPtr<ID3D12Resource> vertexBufferUploadHeap;
    ComPtr<ID3D12Resource> indexBufferUploadHeap;

    // Upload vertex data to the Default Heap and create a Vertex Buffer View of the
    // resource.
    m_vertexBufferView = this->UploadVertexDataToDefaultHeap(
        m_device.Get(),
        copyCommandList.Get(),
        vertexBufferUploadHeap,
        m_vertexBuffer
        );

    // Upload index data to the Default Heap and create an Index Buffer View of the
    // resource.
    m_indexBufferView = this->UploadIndexDataToDefaultHeap(
        m_device.Get(),
        copyCommandList.Get(),
        indexBufferUploadHeap,
        m_indexBuffer
        );

    // Close the command list and execute it to begin the initial GPU setup.
    ThrowIfFailed(
        copyCommandList->Close()
        );
    std::vector<ID3D12CommandList*> commandLists = { copyCommandList.Get() };
    m_commandQueue->ExecuteCommandLists(uint(commandLists.size()), commandLists.data());


    // Create synchronization objects and wait until assets have been uploaded to the GPU.
    {
        ThrowIfFailed(
            m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence))
            );
        m_fenceValue = 1;

        // Create an event handle to use for frame synchronization.
        m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (m_fenceEvent == nullptr) {
            ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
        }

        // Wait for the command list to execute; we are reusing the same command 
        // list in our main loop but for now, we just want to wait for setup to 
        // complete before continuing.
        WaitForPreviousFrame();
    }
}

//---------------------------------------------------------------------------------------
// Update frame-based values.
void IndexRendering::OnUpdate()
{

}

//---------------------------------------------------------------------------------------
// Render the scene.
void IndexRendering::OnRender()
{
	// Execute the command list.
	std::vector<ID3D12CommandList*> commandLists;
    for (auto & cmdList : m_drawCommandList) {
        commandLists.push_back(cmdList.Get());
    }
	m_commandQueue->ExecuteCommandLists(1, &commandLists[m_frameIndex]);

	// Present the frame.
	ThrowIfFailed (
        m_swapChain->Present(1, 0)
    );

	WaitForPreviousFrame();
}

//---------------------------------------------------------------------------------------
void IndexRendering::OnDestroy()
{
	// Ensure that the GPU is no longer referencing resources that are about to be
	// cleaned up by the destructor.
	WaitForPreviousFrame();

	CloseHandle(m_fenceEvent);
}

//---------------------------------------------------------------------------------------
void IndexRendering::PopulateCommandList()
{
    // Command list allocators can only be reset when the associated 
    // command lists have finished execution on the GPU; apps should use 
    // fences to determine GPU execution progress.
    ThrowIfFailed(m_commandAllocator->Reset());

    for (int i(0); i < FrameCount; ++i) {

        // However, when ExecuteCommandList() is called on a particular command 
        // list, that command list can then be reset at any time and must be before 
        // re-recording.
        ThrowIfFailed(
            m_drawCommandList[i]->Reset(m_commandAllocator.Get(), m_pipelineState.Get())
         );


        // Set necessary state.
        m_drawCommandList[i]->SetGraphicsRootSignature(m_rootSignature.Get());
        m_drawCommandList[i]->RSSetViewports(1, &m_viewport);
        m_drawCommandList[i]->RSSetScissorRects(1, &m_scissorRect);

        // Indicate that the back buffer will be used as a render target.
        m_drawCommandList[i]->ResourceBarrier (1,
            &CD3DX12_RESOURCE_BARRIER::Transition (
                m_renderTargets[i].Get(),
                D3D12_RESOURCE_STATE_PRESENT,
                D3D12_RESOURCE_STATE_RENDER_TARGET
            )
        );

        int descOffset = i;
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle (
            m_rtvHeap->GetCPUDescriptorHandleForHeapStart(),
            descOffset, 
            m_rtvDescriptorSize
        );
        m_drawCommandList[i]->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

        // Record commands.
        const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
        m_drawCommandList[i]->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
        m_drawCommandList[i]->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        m_drawCommandList[i]->IASetVertexBuffers(0, 1, &m_vertexBufferView);
        m_drawCommandList[i]->IASetIndexBuffer(&m_indexBufferView);
        m_drawCommandList[i]->DrawIndexedInstanced(m_indexCount, 1, 0, 0, 0);

        // Indicate that the back buffer will now be used to present.
        m_drawCommandList[i]->ResourceBarrier (1,
            &CD3DX12_RESOURCE_BARRIER::Transition (
                m_renderTargets[i].Get(),
                D3D12_RESOURCE_STATE_RENDER_TARGET,
                D3D12_RESOURCE_STATE_PRESENT
            )
        );

        ThrowIfFailed(m_drawCommandList[i]->Close());
    }
}

//---------------------------------------------------------------------------------------
void IndexRendering::WaitForPreviousFrame()
{
	// WAITING FOR THE FRAME TO COMPLETE BEFORE CONTINUING IS NOT BEST PRACTICE.
	// This is code implemented as such for simplicity. The D3D12HelloFrameBuffering
	// sample illustrates how to use fences for efficient resource usage and to
	// maximize GPU utilization.

	// Signal and increment the fence value.
	const uint64 fence = m_fenceValue;
	ThrowIfFailed (
        m_commandQueue->Signal(m_fence.Get(), fence)
    );
	m_fenceValue++;

	// Wait until the previous frame is finished.
	if (m_fence->GetCompletedValue() < fence)
	{
		ThrowIfFailed (
            m_fence->SetEventOnCompletion(fence, m_fenceEvent)
        );
		WaitForSingleObject(m_fenceEvent, INFINITE);
	}

	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
}
