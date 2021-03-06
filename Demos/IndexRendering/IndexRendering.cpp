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
    std::string name
)   
    :   D3D12DemoBase(width, height, name)
{

}


//---------------------------------------------------------------------------------------
void IndexRendering::InitializeDemo (
	ID3D12GraphicsCommandList * uploadCmdList
) {
	LoadAssets(uploadCmdList);
}

//---------------------------------------------------------------------------------------
void IndexRendering::LoadAssets (
	ID3D12GraphicsCommandList * uploadCmdList
) {
	// Create an empty root signature.
	CreateRootSignature();

    // Load shader bytecode.
	LoadCompiledShaderFromFile (GetAssetPath ("VertexShader.cso").c_str (), m_vertexShader);
	LoadCompiledShaderFromFile (GetAssetPath ("PixelShader.cso").c_str (), m_pixelShader);

	// Create the pipeline state object.
    CreatePipelineState(m_vertexShader, m_pixelShader);

    CreateVertexDataBuffers(uploadCmdList);
}

//---------------------------------------------------------------------------------------
void IndexRendering::CreateRootSignature()
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
void IndexRendering::CreatePipelineState(
	const ShaderSource & vertexShader,
	const ShaderSource & pixelShader
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
    psoDesc.VS = vertexShader.byteCode;
    psoDesc.PS = pixelShader.byteCode;
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
	SET_D3D12_DEBUG_NAME(m_pipelineState);
}

//---------------------------------------------------------------------------------------
void IndexRendering::CreateVertexDataBuffers (
	ID3D12GraphicsCommandList * uploadCmdList
) {
    // Upload buffers (which reside in the upload Heap) are only needed when loading data
    // into GPU memory.
    // Note: ComPtr's are CPU objects but this resource needs to stay in scope until
    // the command list that references it has finished executing on the GPU.
    // We will flush the GPU at the end of this method to ensure the resource is not
    // prematurely destroyed.
    static ComPtr<ID3D12Resource> uploadBuffer;

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
		SET_D3D12_DEBUG_NAME(m_vertexBuffer);

		const auto indexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer (sizeof(indices));
		m_device->CreateCommittedResource (
			&defaultHeapProperties,
			D3D12_HEAP_FLAG_NONE,
			&indexBufferDesc,
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS (&m_indexBuffer)
		);
		SET_D3D12_DEBUG_NAME(m_indexBuffer);
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
		uploadCmdList->CopyBufferRegion (
			m_vertexBuffer.Get(), dstOffset, uploadBuffer.Get(), srcOffset, sizeof(vertices)
		);

		dstOffset = 0;
		srcOffset = sizeof(vertices);
		uploadCmdList->CopyBufferRegion (
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

		uploadCmdList->ResourceBarrier(2, barriers);
	}
}

//---------------------------------------------------------------------------------------
void IndexRendering::Update()
{

}

//---------------------------------------------------------------------------------------
void IndexRendering::Render (
	ID3D12GraphicsCommandList * drawCmdList
) {
	// Set necessary state.
	drawCmdList->SetPipelineState(m_pipelineState.Get());
	drawCmdList->SetGraphicsRootSignature(m_rootSignature.Get());

	drawCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	drawCmdList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
	drawCmdList->IASetIndexBuffer(&m_indexBufferView);

	drawCmdList->DrawIndexedInstanced(m_indexCount, 1, 0, 0, 0);
}
