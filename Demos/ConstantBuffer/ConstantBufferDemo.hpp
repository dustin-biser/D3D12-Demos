#pragma once

#include <memory>
#include <vector>
#include <DirectXMath.h>

#include "Common/D3D12DemoBase.hpp"
#include "Common/ResourceUploadBuffer.hpp"
#include "Common/ShaderUtils.hpp"

#include "ConstantBufferDefines.hpp"


class ConstantBufferDemo : public D3D12DemoBase {
public:
	ConstantBufferDemo (
        uint width,
        uint height,
        std::string name
    );

	void InitializeDemo (
		ID3D12GraphicsCommandList * uploadCmdList
	) override;

	void Update() override;

	void Render (
		ID3D12GraphicsCommandList * drawCmdList
	) override;


private:
	struct Vertex {
		float position[3];
		float normal[3];
	};
	typedef ushort Index;

	// Constant Buffer specific
	ComPtr<ID3D12DescriptorHeap> m_cbvDescHeap;
	SceneConstants m_sceneConstData[NUM_BUFFERED_FRAMES];
	PointLight m_pointLightConstData[NUM_BUFFERED_FRAMES];

	D3D12_CONSTANT_BUFFER_VIEW_DESC m_cbvDesc_PointLight[NUM_BUFFERED_FRAMES];
	void * m_cbv_PointLight_dataPtr[NUM_BUFFERED_FRAMES];

	D3D12_CONSTANT_BUFFER_VIEW_DESC m_cbvDesc_SceneConstants[NUM_BUFFERED_FRAMES];
	void * m_cbv_SceneConstants_dataPtr[NUM_BUFFERED_FRAMES];

	// Pipeline objects.
	ComPtr<ID3D12RootSignature> m_rootSignature;
	ComPtr<ID3D12PipelineState> m_pipelineState;

	// App resources.
	std::vector<Vertex> m_vertexArray;
	std::vector<ushort> m_indexArray;
	D3D12_INPUT_LAYOUT_DESC m_inputLayoutDesc;
	std::shared_ptr<ResourceUploadBuffer> m_uploadBuffer;

	ComPtr<ID3D12Resource> m_vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
	ComPtr<ID3D12Resource> m_indexBuffer;
	D3D12_INDEX_BUFFER_VIEW m_indexBufferView;
    uint m_indexCount;

	ShaderSource m_vertexShader;
	ShaderSource m_pixelShader;

	void LoadPipelineDependencies();

	void LoadAssets ();

	void PopulateCommandList();

	void UpdateConstantBuffers();

	void CreatePipelineState (
		const ShaderSource & vertexShader,
		const ShaderSource & pixelShader
	);
};

