#include "pch.h"

#include <vector>
#include <iostream>
#include "IndexRendering.hpp"

using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace std;


//---------------------------------------------------------------------------------------
IndexRendering::IndexRendering (
    uint width, 
    uint height,
    std::wstring name
)   
    :   D3D12DemoBase(width, height, name),
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
void IndexRendering::initializeDemo()
{
	loadRenderPipelineDependencies();
	loadAssets();
}


//---------------------------------------------------------------------------------------
// Loads the rendering pipeline dependencies such as device, command queue, swap chain,
// render target views and command allocator.
void IndexRendering::loadRenderPipelineDependencies()
{
    // Create RTVs, one for each SwapChain buffer.
    this->createRenderTargetView (
        m_device.Get(),
        m_swapChain.Get(),
        m_rtvHeap,
        m_rtvDescriptorSize,
        m_renderTargets
    );
    NAME_D3D12_OBJECT(m_renderTargets[0]);
    NAME_D3D12_OBJECT(m_renderTargets[1]);

    // Create command allocator for managing command list memory
	for (int i(0); i < NUM_BUFFERED_FRAMES; ++i) {
		CHECK_D3D_RESULT(
			m_device->CreateCommandAllocator(
				D3D12_COMMAND_LIST_TYPE_DIRECT,
				IID_PPV_ARGS(&m_commandAllocator[i])
			)
		);
	}
    NAME_D3D12_OBJECT_ARRAY(m_commandAllocator, NUM_BUFFERED_FRAMES);


    // Create the draw command lists which will hold our rendering commands.
	// Create one command list for each swap chain buffer.
    for (uint i(0); i < NUM_BUFFERED_FRAMES; ++i) {
        CHECK_D3D_RESULT(
            m_device->CreateCommandList (
                0,
                D3D12_COMMAND_LIST_TYPE_DIRECT,
                m_commandAllocator[i].Get(),
                nullptr, // Will set pipeline state later before drawing
                IID_PPV_ARGS(&m_drawCommandList[i])
            )
        );
        // Stop recording, will reset this later before issuing drawing commands.
        m_drawCommandList[i]->Close();
    }
    NAME_D3D12_OBJECT_ARRAY(m_drawCommandList, NUM_BUFFERED_FRAMES);


	// Create copy command allocator for managing memory for copy command list.
	CHECK_D3D_RESULT(
		m_device->CreateCommandAllocator(
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			IID_PPV_ARGS(&m_copyCommandAllocator)
		)
	);
    NAME_D3D12_OBJECT(m_copyCommandAllocator);


    // Create a separate command list for copying resource data to the GPU.
    CHECK_D3D_RESULT (
        m_device->CreateCommandList (
            0,
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            m_copyCommandAllocator.Get(),
            nullptr,
            IID_PPV_ARGS(&m_copyCommandList)
         )
    );
    NAME_D3D12_OBJECT(m_copyCommandList);
}


//---------------------------------------------------------------------------------------
void IndexRendering::createRenderTargetView (
    _In_ ID3D12Device * device,
    _In_ IDXGISwapChain * swapChain,
    _Out_ ComPtr<ID3D12DescriptorHeap> & rtvHeap,
    _Out_ uint & rtvDescriptorSize,
    _Out_ ComPtr<ID3D12Resource> * renderTargets
) {
    // Describe and create a render target view (RTV) descriptor heap which will
    // hold the RTV descriptors.
	{
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
		rtvHeapDesc.NumDescriptors = NUM_BUFFERED_FRAMES;
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		CHECK_D3D_RESULT (
            device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap))
        );

		rtvDescriptorSize =
            device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}

	// Create a RTV for each swapChain buffer.
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle (
            rtvHeap->GetCPUDescriptorHandleForHeapStart()
        );

        // Create a RTV with sRGB format so output image is properly gamma corrected.
        D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
        rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

		// Create a RenderTargetView for each frame.
		for (uint n(0); n < NUM_BUFFERED_FRAMES; ++n)
		{
			CHECK_D3D_RESULT (
                swapChain->GetBuffer(n, IID_PPV_ARGS(&renderTargets[n]))
            );
			device->CreateRenderTargetView(renderTargets[n].Get(), &rtvDesc, rtvHandle);
			rtvHandle.Offset(1, rtvDescriptorSize);
		}
	}

}

//---------------------------------------------------------------------------------------
// Load the sample assets.
void IndexRendering::loadAssets()
{
	// Create an empty root signature.
    this->createRootSignature(
        m_device.Get(),
        m_rootSignature
    );

    // Load shader bytecode.
    ComPtr<ID3DBlob> vertexShaderBlob;
    ComPtr<ID3DBlob> pixelShaderBlob;
    this->loadShaders(vertexShaderBlob, pixelShaderBlob);

	// Create the pipeline state object.
    this->createPipelineState (
        m_device.Get(),
        m_rootSignature.Get(),
        vertexShaderBlob.Get(),
        pixelShaderBlob.Get(),
        m_pipelineState
    ); NAME_D3D12_OBJECT(m_pipelineState);

    this->createVertexDataBuffers (
        m_device.Get(),
        m_directCmdQueue.Get(),
        m_copyCommandList.Get(),
        m_vertexBuffer,
        m_indexBuffer,
        m_indexCount
    );
    NAME_D3D12_OBJECT(m_vertexBuffer);
    NAME_D3D12_OBJECT(m_indexBuffer);
}

//---------------------------------------------------------------------------------------
void IndexRendering::createRootSignature (
    _In_ ID3D12Device * device,
    _Out_ ComPtr<ID3D12RootSignature> & rootSignature
) {
    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Init (
        0, nullptr, 0, nullptr,
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
    );

    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;

    CHECK_D3D_RESULT (
        D3D12SerializeRootSignature (
            &rootSignatureDesc,
            D3D_ROOT_SIGNATURE_VERSION_1, 
            &signature, 
            &error
        )
    );

    CHECK_D3D_RESULT (
        device->CreateRootSignature (
            0,
            signature->GetBufferPointer(),
            signature->GetBufferSize(),
            IID_PPV_ARGS(&rootSignature)
        )
    );
}


//---------------------------------------------------------------------------------------
void IndexRendering::loadShaders (
    _Out_ ComPtr<ID3DBlob> & vertexShaderBlob,
    _Out_ ComPtr<ID3DBlob> & pixelShaderBlob
) {
    D3DReadFileToBlob(getAssetPath(L"VertexShader.cso").c_str(), &vertexShaderBlob);
    D3DReadFileToBlob(getAssetPath(L"PixelShader.cso").c_str(), &pixelShaderBlob);
}

//---------------------------------------------------------------------------------------
void IndexRendering::createPipelineState(
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

    // Colors
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
    psoDesc.pRootSignature = rootSignature;
    psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShaderBlob);
    psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShaderBlob);
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
    CHECK_D3D_RESULT (
        device->CreateGraphicsPipelineState (
            &psoDesc,
            IID_PPV_ARGS(&pipelineState)
        )
     );
}

//---------------------------------------------------------------------------------------
D3D12_VERTEX_BUFFER_VIEW IndexRendering::uploadVertexDataToDefaultHeap(
    _In_ ID3D12Device * device,
    _In_ ID3D12GraphicsCommandList * copyCommandList,
    _Out_ ComPtr<ID3D12Resource> & vertexUploadBuffer,
    _Out_ ComPtr<ID3D12Resource> & vertexBuffer
) {
	const float aspectRatio = static_cast<float>(m_windowWidth) / m_windowHeight;

    // Define vertices for a square.
    const Vertex vertexData[] =
    {
        // Positions                              Colors
        { { 0.25f, 0.25f * aspectRatio, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
        { { 0.25f, -0.25f * aspectRatio, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
        { { -0.25f, -0.25f * aspectRatio, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } },
        { { -0.25f, 0.25f * aspectRatio, 0.0f }, { 1.0f, 0.0f, 1.0f, 1.0f } }
    };
    const uint vertexBufferSize = sizeof(vertexData);

    // The vertex buffer resource will live in the Default Heap, and will
    // be the copy destination.
    CHECK_D3D_RESULT(
        device->CreateCommittedResource (
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize),
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_PPV_ARGS(&vertexBuffer))
    );

    // The Upload Heap will contain the raw vertex data, and will be the copy source.
    CHECK_D3D_RESULT(
        device->CreateCommittedResource (
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize),
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&vertexUploadBuffer)
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
            vertexUploadBuffer.Get(),
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
D3D12_INDEX_BUFFER_VIEW IndexRendering::uploadIndexDataToDefaultHeap(
    _In_ ID3D12Device * device,
    _In_ ID3D12GraphicsCommandList * copyCommandList,
    _Out_ ComPtr<ID3D12Resource> & indexUploadBuffer,
    _Out_ ComPtr<ID3D12Resource> & indexBuffer,
    _Out_ uint & indexCount
) {
    std::vector<ushort> indices = { 2, 1,0, 2,0,3 };
    indexCount = uint(indices.size());
    const uint indexBufferSize = sizeof(ushort) * indexCount;

    CHECK_D3D_RESULT(
        device->CreateCommittedResource (
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize),
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_PPV_ARGS(&indexBuffer))
    );

    CHECK_D3D_RESULT(
        device->CreateCommittedResource (
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize),
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&indexUploadBuffer))
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
            indexUploadBuffer.Get(),
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
void IndexRendering::createVertexDataBuffers (
    _In_ ID3D12Device * device,
    _In_ ID3D12CommandQueue * commandQueue,
    _In_ ID3D12GraphicsCommandList * copyCommandList,
    _Out_ ComPtr<ID3D12Resource> & vertexBuffer,
    _Out_ ComPtr<ID3D12Resource> & indexBuffer,
    _Out_ uint & indexCount
) {
    // Upload buffers (which rides in the Upload Heap) are only needed when loading data
    // into GPU memory.
    // Note: ComPtr's are CPU objects but this resource needs to stay in scope until
    // the command list that references it has finished executing on the GPU.
    // We will flush the GPU at the end of this method to ensure the resource is not
    // prematurely destroyed.
    ComPtr<ID3D12Resource> vertexUploadBuffer;
    ComPtr<ID3D12Resource> indexUploadBuffer;

    // Upload vertex data to the Default Heap and create a Vertex Buffer View of the
    // resource.
    m_vertexBufferView = this->uploadVertexDataToDefaultHeap (
        device,
        copyCommandList,
        vertexUploadBuffer,
        vertexBuffer
    );

    // Upload index data to the Default Heap and create an Index Buffer View of the
    // resource.
    m_indexBufferView = this->uploadIndexDataToDefaultHeap (
        device,
        copyCommandList,
        indexUploadBuffer,
        indexBuffer,
        indexCount
    );

    // Close the command list and execute it to begin the initial GPU setup.
    CHECK_D3D_RESULT(
        copyCommandList->Close()
    );
    std::vector<ID3D12CommandList*> commandLists = { copyCommandList };
    commandQueue->ExecuteCommandLists(uint(commandLists.size()), commandLists.data());

	waitForGpuCompletion(m_directCmdQueue.Get());
}

//---------------------------------------------------------------------------------------
void IndexRendering::update()
{

}

//---------------------------------------------------------------------------------------
void IndexRendering::render()
{
	populateCommandList();

	// Execute the command list.
	ID3D12CommandList* commandLists[] = { m_drawCommandList[m_frameIndex].Get() };
	m_directCmdQueue->ExecuteCommandLists(_countof(commandLists), commandLists);
}

//---------------------------------------------------------------------------------------
void IndexRendering::populateCommandList()
{
    CHECK_D3D_RESULT(
        m_commandAllocator[m_frameIndex]->Reset()
    );

    for (int i(0); i < NUM_BUFFERED_FRAMES; ++i) {

        CHECK_D3D_RESULT(
            m_drawCommandList[i]->Reset(m_commandAllocator[m_frameIndex].Get(), m_pipelineState.Get())
         );


        // Set necessary state.
        m_drawCommandList[i]->SetPipelineState(m_pipelineState.Get());
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

        CHECK_D3D_RESULT(m_drawCommandList[i]->Close());
    }
}

//---------------------------------------------------------------------------------------
void IndexRendering::cleanupDemo()
{

}

