#include "pch.h"

#include "ConstantBufferDemo.hpp"
using namespace DirectX;
using Microsoft::WRL::ComPtr;

#include <vector>
#include <iostream>
using namespace std;


//---------------------------------------------------------------------------------------
ConstantBufferDemo::ConstantBufferDemo (
    uint width, 
    uint height,
    std::string name
)   
    :   D3D12DemoBase(width, height, name)
{

}

//---------------------------------------------------------------------------------------
void ConstantBufferDemo::InitializeDemo (
	ID3D12GraphicsCommandList * uploadCmdList
) {
	LoadPipelineDependencies();

	LoadAssets();
}

//---------------------------------------------------------------------------------------
// Loads the rendering pipeline dependencies.
void ConstantBufferDemo::LoadPipelineDependencies()
{
	//-- Describe and create the CBV Descriptor Heap.
	{
		D3D12_DESCRIPTOR_HEAP_DESC cbvDescHeapDescriptor = {};
		cbvDescHeapDescriptor.NumDescriptors = 2 * NUM_BUFFERED_FRAMES;
		cbvDescHeapDescriptor.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		cbvDescHeapDescriptor.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		CHECK_D3D_RESULT (
			m_device->CreateDescriptorHeap(&cbvDescHeapDescriptor, IID_PPV_ARGS(&m_cbvDescHeap))
		);
	}
}

//---------------------------------------------------------------------------------------
void ConstantBufferDemo::LoadAssets ()
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

		CHECK_D3D_RESULT(
			D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1,
				&signature, &error)
		);

		CHECK_D3D_RESULT(
			m_device->CreateRootSignature(0, signature->GetBufferPointer(),
				signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature))
		);
	}

	//-- Load shader byte code:
	LoadCompiledShaderFromFile(GetAssetPath("VertexShader.cso").c_str(), m_vertexShader);
	LoadCompiledShaderFromFile(GetAssetPath("PixelShader.cso").c_str(), m_pixelShader);

	// Create the pipeline state object.
	CreatePipelineState(m_vertexShader, m_pixelShader);

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
	for (int i(0); i < NUM_BUFFERED_FRAMES; ++i) {
		ZeroMemory(&m_sceneConstData[i], sizeof(SceneConstants));
		m_uploadBuffer->uploadConstantBufferData(
			reinterpret_cast<const void *>(&m_sceneConstData[i]),
			sizeof(SceneConstants),
			m_cbvDesc_SceneConstants[i],
			&m_cbv_SceneConstants_dataPtr[i]
		);
	}

	// Create PointLight ConstantBuffer storage within upload heap, duplicating
	// storage space for each buffered frame.
	for (int i(0); i < NUM_BUFFERED_FRAMES; ++i) {
		ZeroMemory(&m_pointLightConstData[i], sizeof(PointLight));
		m_uploadBuffer->uploadConstantBufferData (
			reinterpret_cast<const void *>(&m_pointLightConstData[i]),
			sizeof(PointLight),
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
}


//---------------------------------------------------------------------------------------
void ConstantBufferDemo::CreatePipelineState (
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
    psoDesc.pRootSignature = m_rootSignature.Get();
    psoDesc.VS = vertexShader.byteCode;
    psoDesc.PS = pixelShader.byteCode;
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
	SET_D3D12_DEBUG_NAME(m_pipelineState);
}

//---------------------------------------------------------------------------------------
void ConstantBufferDemo::UpdateConstantBuffers()
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
		memcpy (m_cbv_SceneConstants_dataPtr[m_frameIndex], 
				&m_sceneConstData[m_frameIndex],
				sizeof(SceneConstants));

		// Update PointLight ConstantBuffer data:
		memcpy (m_cbv_PointLight_dataPtr[m_frameIndex], 
				&m_pointLightConstData[m_frameIndex],
				sizeof(PointLight) );
	}
}

//---------------------------------------------------------------------------------------
void ConstantBufferDemo::Update()
{
	this->UpdateConstantBuffers();
}

//---------------------------------------------------------------------------------------
void ConstantBufferDemo::Render (
	ID3D12GraphicsCommandList * drawCmdList
) {
	drawCmdList->SetPipelineState(m_pipelineState.Get());
	drawCmdList->SetGraphicsRootSignature(m_rootSignature.Get());

	ID3D12DescriptorHeap * ppHeaps[] = {m_cbvDescHeap.Get()};
	drawCmdList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
	drawCmdList->SetGraphicsRootConstantBufferView (
		0, m_cbvDesc_SceneConstants[m_frameIndex].BufferLocation
	);
	drawCmdList->SetGraphicsRootConstantBufferView (
		1, m_cbvDesc_PointLight[m_frameIndex].BufferLocation
	);

	drawCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	const uint inputSlot0 = 0;
	drawCmdList->IASetVertexBuffers(inputSlot0, 1, &m_vertexBufferView);
	drawCmdList->IASetIndexBuffer(&m_indexBufferView);

	UINT numIndices = static_cast<UINT>(m_indexArray.size());
	drawCmdList->DrawIndexedInstanced(numIndices, 1, 0, 0, 0);
}

//---------------------------------------------------------------------------------------
void ConstantBufferDemo::PopulateCommandList()
{
}
