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
    :   D3D12DemoBase(width, height, name)
{

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
    createRenderTargetView();

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
void IndexRendering::createRenderTargetView()
{
    // Describe and create a render target view (RTV) descriptor heap which will
    // hold the RTV descriptors.
	{
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
		rtvHeapDesc.NumDescriptors = NUM_BUFFERED_FRAMES;
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		CHECK_D3D_RESULT (
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
		for (uint n(0); n < NUM_BUFFERED_FRAMES; ++n)
		{
			CHECK_D3D_RESULT (
                m_swapChain->GetBuffer(n, IID_PPV_ARGS(&m_renderTargets[n]))
            );
			m_device->CreateRenderTargetView(m_renderTargets[n].Get(), &rtvDesc, rtvHandle);
			rtvHandle.Offset(1, m_rtvDescriptorSize);
		}
		NAME_D3D12_OBJECT_ARRAY(m_renderTargets, 2);
	}
}

//---------------------------------------------------------------------------------------
// Load the sample assets.
void IndexRendering::loadAssets()
{
	// Create an empty root signature.
	createRootSignature();

    // Load shader bytecode.
    ComPtr<ID3DBlob> vertexShaderBlob;
    ComPtr<ID3DBlob> pixelShaderBlob;
    loadShaders(vertexShaderBlob, pixelShaderBlob);

	// Create the pipeline state object.
    createPipelineState(vertexShaderBlob.Get(), pixelShaderBlob.Get());

    createVertexDataBuffers();
}

//---------------------------------------------------------------------------------------
void IndexRendering::createRootSignature()
{
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
        m_device->CreateRootSignature (
            0,
            signature->GetBufferPointer(),
            signature->GetBufferSize(),
            IID_PPV_ARGS(&m_rootSignature)
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
    _In_ ID3DBlob * vertexShaderBlob,
    _In_ ID3DBlob * pixelShaderBlob
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
    psoDesc.pRootSignature = m_rootSignature.Get();
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
        m_device->CreateGraphicsPipelineState (
            &psoDesc,
            IID_PPV_ARGS(&m_pipelineState)
        )
     );
	NAME_D3D12_OBJECT(m_pipelineState);
}

//---------------------------------------------------------------------------------------
void IndexRendering::createVertexDataBuffers ()
{
    // Upload buffers (which reside in the upload Heap) are only needed when loading data
    // into GPU memory.
    // Note: ComPtr's are CPU objects but this resource needs to stay in scope until
    // the command list that references it has finished executing on the GPU.
    // We will flush the GPU at the end of this method to ensure the resource is not
    // prematurely destroyed.
    ComPtr<ID3D12Resource> uploadBuffer;

	const float aspectRatio = static_cast<float>(m_windowWidth) / m_windowHeight;

	// Define vertices for a square.
	const Vertex vertices[] =
	{
		  // Positions                          Colors
		{ {0.25f, 0.25f * aspectRatio, 0.0f},   {1.0f, 0.0f, 0.0f, 1.0f} },
		{ {0.25f, -0.25f * aspectRatio, 0.0f},  {0.0f, 1.0f, 0.0f, 1.0f} },
		{ {-0.25f, -0.25f * aspectRatio, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f} },
		{ {-0.25f, 0.25f * aspectRatio, 0.0f},  {1.0f, 0.0f, 1.0f, 1.0f} }
	};

    const ushort indices[] = { 2,1,0, 2,0,3 };
	m_indexCount = _countof(indices);

	const int uploadBufferSize = sizeof(vertices) + sizeof(indices);
	const auto uploadHeapProperties = CD3DX12_HEAP_PROPERTIES (D3D12_HEAP_TYPE_UPLOAD);
	const auto uploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer (uploadBufferSize);

	// Create upload buffer.
	m_device->CreateCommittedResource (
		&uploadHeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&uploadBufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS (&uploadBuffer)
	);

	// Allocate vertex and index buffers within the default heap
	{
		const auto defaultHeapProperties = CD3DX12_HEAP_PROPERTIES (D3D12_HEAP_TYPE_DEFAULT);

		const auto vertexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer (sizeof(vertices));
		m_device->CreateCommittedResource (
			&defaultHeapProperties,
			D3D12_HEAP_FLAG_NONE,
			&vertexBufferDesc,
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS (&m_vertexBuffer)
		);
		NAME_D3D12_OBJECT(m_vertexBuffer);

		const auto indexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer (sizeof(indices));
		m_device->CreateCommittedResource (
			&defaultHeapProperties,
			D3D12_HEAP_FLAG_NONE,
			&indexBufferDesc,
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS (&m_indexBuffer)
		);
		NAME_D3D12_OBJECT(m_indexBuffer);
	}

	// Create buffer views
	m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
	m_vertexBufferView.SizeInBytes = sizeof(vertices);
	m_vertexBufferView.StrideInBytes = sizeof(Vertex);

	m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
	m_indexBufferView.SizeInBytes = sizeof(indices);
	m_indexBufferView.Format = DXGI_FORMAT_R16_UINT;

	// Copy data in CPU memory to upload buffer
	{
		void * p;
		// Set read range to zero, since we are only going to upload data to upload buffer
		// rather than read data from it.
		D3D12_RANGE readRange = {0, 0};
		uploadBuffer->Map(0, &readRange, &p);
		::memcpy(p, vertices, sizeof(vertices));
		::memcpy(static_cast<byte *>(p) + sizeof(vertices), indices, sizeof(indices));

		// Finished uploading data to uploadBuffer, so unmap it.
		D3D12_RANGE writtenRange = {0, sizeof(vertices) + sizeof(indices)};
		uploadBuffer->Unmap(0, &writtenRange);
	}

	// Copy data from upload buffer on CPU into the index/vertex buffer on 
	// the GPU.
	{
		uint64 dstOffset = 0;
		uint64 srcOffset = 0;
		m_copyCommandList->CopyBufferRegion (
			m_vertexBuffer.Get(), dstOffset, uploadBuffer.Get(), srcOffset, sizeof(vertices)
		);

		dstOffset = 0;
		srcOffset = sizeof(vertices);
		m_copyCommandList->CopyBufferRegion (
			m_indexBuffer.Get(), dstOffset, uploadBuffer.Get(), srcOffset, sizeof(indices)
		);
	}

	// Batch resource barriers marking state transitions.
	{
		const CD3DX12_RESOURCE_BARRIER barriers[2] = {

			CD3DX12_RESOURCE_BARRIER::Transition (
				m_vertexBuffer.Get(),
				D3D12_RESOURCE_STATE_COPY_DEST,
				D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER
			),

			CD3DX12_RESOURCE_BARRIER::Transition (
				m_indexBuffer.Get(),
				D3D12_RESOURCE_STATE_COPY_DEST,
				D3D12_RESOURCE_STATE_INDEX_BUFFER
			)
		};

		m_copyCommandList->ResourceBarrier(2, barriers);
	}

    // Close command list and execute it on the command queue. 
    CHECK_D3D_RESULT (
        m_copyCommandList->Close()
    );
    ID3D12CommandList * commandLists[] = { m_copyCommandList.Get() };
    m_directCmdQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

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

