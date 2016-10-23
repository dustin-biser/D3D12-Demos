#include "pch.h"

#include "TextureDemo.hpp"
using namespace DirectX;
using Microsoft::WRL::ComPtr;

#include <vector>
#include <iostream>
using namespace std;

#include "Common/ImageIO.hpp"


//---------------------------------------------------------------------------------------
TextureDemo::TextureDemo (
    uint windowWidth, 
    uint windowHeight,
    std::wstring windowTitle
)   
    :   D3D12DemoBase(windowWidth, windowHeight, windowTitle)
{

}


//---------------------------------------------------------------------------------------
TextureDemo::~TextureDemo()
{
	m_rootSignature->Release();
	m_pipelineState->Release();
}

//---------------------------------------------------------------------------------------
void TextureDemo::initializeDemo()
{
	createRootSignature();

	createConstantBuffers();

	uploadVertexDataToGpu();

	createTexture();

	//-- Load shader byte code:
	ComPtr<ID3DBlob> vertexShaderBlob;
	ComPtr<ID3DBlob> pixelShaderBlob;
	D3DReadFileToBlob(getAssetPath(L"VertexShader.cso").c_str(), &vertexShaderBlob);
	D3DReadFileToBlob(getAssetPath(L"PixelShader.cso").c_str(), &pixelShaderBlob);

	// Create the pipeline state object.
	createPipelineState(vertexShaderBlob.Get(), pixelShaderBlob.Get());
}


//---------------------------------------------------------------------------------------
void TextureDemo::createRootSignature()
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

	CHECK_D3D_RESULT(
		D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1,
			&signature, &error)
	);

	CHECK_D3D_RESULT(
		m_device->CreateRootSignature(0, signature->GetBufferPointer(),
			signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature))
	);

}


//---------------------------------------------------------------------------------------
void TextureDemo::createConstantBuffers()
{
	//-- Create SceneConstant constant buffers within upload heap:
	for (int i(0); i < NUM_BUFFERED_FRAMES; ++i) {
		const auto uploadHeapProperties = CD3DX12_HEAP_PROPERTIES (D3D12_HEAP_TYPE_UPLOAD);
		const auto constantBufferDesc = CD3DX12_RESOURCE_DESC::Buffer (
			sizeof (SceneConstants)
		);

		m_device->CreateCommittedResource (
			&uploadHeapProperties,
			D3D12_HEAP_FLAG_NONE,
			&constantBufferDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS (&m_constantBuffer_sceneConstant[i])
		);

		ZeroMemory(&m_sceneConstData[i], sizeof(SceneConstants));

		void * p;
		m_constantBuffer_sceneConstant[i]->Map (0, nullptr, &p);
		::memcpy(p, &m_sceneConstData[i], sizeof (SceneConstants));
		m_constantBuffer_sceneConstant[i]->Unmap (0, nullptr);
	}

	//-- Create PointLight constant buffers within upload heap:
	for (int i(0); i < NUM_BUFFERED_FRAMES; ++i) {
		const auto uploadHeapProperties = CD3DX12_HEAP_PROPERTIES (D3D12_HEAP_TYPE_UPLOAD);
		const auto constantBufferDesc = CD3DX12_RESOURCE_DESC::Buffer (
			sizeof (DirectionalLight)
		);

		m_device->CreateCommittedResource (
			&uploadHeapProperties,
			D3D12_HEAP_FLAG_NONE,
			&constantBufferDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS (&m_constantBuffer_pointLight[i])
		);

		ZeroMemory(&m_sceneConstData[i], sizeof(DirectionalLight));

		void * p;
		m_constantBuffer_pointLight[i]->Map (0, nullptr, &p);
		::memcpy(p, &m_sceneConstData[i], sizeof(DirectionalLight));
		m_constantBuffer_pointLight[i]->Unmap (0, nullptr);
	}
	
}

//---------------------------------------------------------------------------------------
void TextureDemo::uploadVertexDataToGpu()
{
	// Create upload buffer for uploading vertex/index data to default heap.
    ComPtr<ID3D12Resource> uploadBuffer_vertexData;

	// Quad vertex data.
	Vertex vertexArray[] = {
		// Positions             Normals               TexCords
		{ -0.5f, -0.5f,  0.0f,   0.0f,  1.0f,  0.0f,   0.0f, 0.0f}, // 0
		{  0.5f, -0.5f,  0.0f,   0.0f,  1.0f,  0.0f,   1.0f, 0.0f}, // 1
		{  0.5f,  0.5f,  0.0f,   0.0f,  1.0f,  0.0f,   1.0f, 1.0f}, // 2
		{ -0.5f,  0.5f,  0.0f,   0.0f,  1.0f,  0.0f,   0.0f, 1.0f}  // 3
	};

	// Create index data
	Index indexArray[] = {
		0,1,2, 0,2,3,
	};
	m_numIndices = std::extent<decltype(indexArray)>::value;


	const int uploadBufferSize = sizeof(vertexArray) + sizeof(indexArray);
	const auto uploadHeapProperties = CD3DX12_HEAP_PROPERTIES (D3D12_HEAP_TYPE_UPLOAD);
	const auto uploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer (uploadBufferSize);

	// Create upload buffer.
	m_device->CreateCommittedResource (
		&uploadHeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&uploadBufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS (&uploadBuffer_vertexData)
	);
	NAME_D3D12_OBJECT(uploadBuffer_vertexData);

	// Allocate vertex and index buffers within the default heap
	{
		const auto defaultHeapProperties = CD3DX12_HEAP_PROPERTIES (D3D12_HEAP_TYPE_DEFAULT);

		const auto vertexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer (sizeof(vertexArray));
		m_device->CreateCommittedResource (
			&defaultHeapProperties,
			D3D12_HEAP_FLAG_NONE,
			&vertexBufferDesc,
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS (&m_vertexBuffer)
		);
		NAME_D3D12_OBJECT(m_vertexBuffer);

		const auto indexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer (sizeof(indexArray));
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

	// Initialize vertex buffer view
	m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
	m_vertexBufferView.SizeInBytes = sizeof(vertexArray);
	m_vertexBufferView.StrideInBytes = sizeof(Vertex);

	// Initialize index buffer view
	m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
	m_indexBufferView.SizeInBytes = sizeof(indexArray);
	m_indexBufferView.Format = DXGI_FORMAT_R16_UINT;
	

	// Copy vertex data in CPU memory to upload buffer
	{
		void * p;
		// Set read range to zero, since we are only going to upload data to upload buffer
		// rather than read data from it.
		D3D12_RANGE readRange = {0, 0};
		uploadBuffer_vertexData->Map(0, &readRange, &p);
		::memcpy(p, vertexArray, sizeof(vertexArray));
		::memcpy(static_cast<byte *>(p) + sizeof(vertexArray), indexArray, sizeof(indexArray));

		// Finished uploading data to uploadBuffer, so unmap it.
		D3D12_RANGE writtenRange = {0, sizeof(vertexArray) + sizeof(indexArray)};
		uploadBuffer_vertexData->Unmap(0, &writtenRange);
	}

	// Copy data from upload buffer on CPU into the index/vertex buffer on 
	// the GPU.
	{
		uint64 dstOffset = 0;
		uint64 srcOffset = 0;
		m_copyCmdList->CopyBufferRegion (
			m_vertexBuffer,
			dstOffset,
			uploadBuffer_vertexData.Get(),
			srcOffset,
			sizeof(vertexArray)
		);

		dstOffset = 0;
		srcOffset = sizeof(vertexArray);
		m_copyCmdList->CopyBufferRegion (
			m_indexBuffer,
			dstOffset,
			uploadBuffer_vertexData.Get(),
			srcOffset,
			sizeof(indexArray)
		);
	}


	// Batch resource barriers marking state transitions.
	{
		const CD3DX12_RESOURCE_BARRIER barriers[2] = {

			CD3DX12_RESOURCE_BARRIER::Transition (
				m_vertexBuffer,
				D3D12_RESOURCE_STATE_COPY_DEST,
				D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER
			),

			CD3DX12_RESOURCE_BARRIER::Transition (
				m_indexBuffer,
				D3D12_RESOURCE_STATE_COPY_DEST,
				D3D12_RESOURCE_STATE_INDEX_BUFFER
			)
		};

		m_copyCmdList->ResourceBarrier(2, barriers);
	}

	// Close command list and execute it on the direct command queue. 
	CHECK_D3D_RESULT (
		m_copyCmdList->Close()
	);
	ID3D12CommandList * commandLists[] = {m_copyCmdList};
	m_directCmdQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

	waitForGpuCompletion(m_directCmdQueue);
}

//---------------------------------------------------------------------------------------
void TextureDemo::createPipelineState (
    ID3DBlob * vertexShaderBlob,
    ID3DBlob * pixelShaderBlob
) {
    // Define the vertex input layout.
    D3D12_INPUT_ELEMENT_DESC inputElementDescriptor[3];

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

    // Normals
    inputElementDescriptor[2].SemanticName = "TEXCOORD";
    inputElementDescriptor[2].SemanticIndex = 0;
    inputElementDescriptor[2].Format = DXGI_FORMAT_R32G32_FLOAT;
    inputElementDescriptor[2].InputSlot = 0;
    inputElementDescriptor[2].AlignedByteOffset = sizeof(float) * 6;
    inputElementDescriptor[2].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
    inputElementDescriptor[2].InstanceDataStepRate = 0;

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
    psoDesc.pRootSignature = m_rootSignature;
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
    CHECK_D3D_RESULT (
        m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState))
     );
	NAME_D3D12_OBJECT(m_pipelineState);
}

//---------------------------------------------------------------------------------------
void TextureDemo::createTexture (
) {

	ImageDecoder::decodeImage(getAssetPath(L"Textures\\uvgrid.jpg"), 1, &m_imageData);

	//************************************
	// TODO Dustin - Finish this method
	//************************************
}

//---------------------------------------------------------------------------------------
void TextureDemo::updateConstantBuffers()
{
	//-- Create and Upload SceneContants Data:
	{
		static float rotationAngle(0.0f);
		const float rotationDelta(1.2f);
		rotationAngle += rotationDelta;

		XMVECTOR axis = XMVectorSet(0.5f, 1.0f, 0.5f, 0.0f);
		XMMATRIX rotate = XMMatrixRotationAxis(axis, XMConvertToRadians(rotationAngle));

		XMMATRIX modelMatrix = XMMatrixMultiply(rotate, XMMatrixTranslation(0.0f, 0.0f, -4.0f));

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
			#undef near
			#undef far 

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


		XMVECTOR lightDirection{ -5.0f, 5.0f,  5.0f, 1.0f };

		// Transform lightPosition into View Space
		lightDirection = XMVector4Transform(lightDirection, viewMatrix);
		XMStoreFloat4(&m_pointLightConstData[m_frameIndex].direction, lightDirection);

		// White light
		m_pointLightConstData[m_frameIndex].color = XMFLOAT4{ 1.0f, 1.0f, 1.0f, 1.0f };
	}
}

//---------------------------------------------------------------------------------------
void TextureDemo::update()
{
	this->updateConstantBuffers();
}

//---------------------------------------------------------------------------------------
void TextureDemo::render (
	ID3D12GraphicsCommandList * drawCmdList
) {
	drawCmdList->SetPipelineState(m_pipelineState);

	drawCmdList->SetGraphicsRootSignature(m_rootSignature);

	//-- Set CBVs within root signature:
	drawCmdList->SetGraphicsRootConstantBufferView (
		0, m_constantBuffer_sceneConstant[m_frameIndex]->GetGPUVirtualAddress()
	);
	drawCmdList->SetGraphicsRootConstantBufferView (
		1, m_constantBuffer_pointLight[m_frameIndex]->GetGPUVirtualAddress()
	);

	drawCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	const uint inputSlot0 = 0;
	drawCmdList->IASetVertexBuffers(inputSlot0, 1, &m_vertexBufferView);
	drawCmdList->IASetIndexBuffer(&m_indexBufferView);

	drawCmdList->DrawIndexedInstanced(m_numIndices, 1, 0, 0, 0);
}
