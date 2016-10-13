#pragma once

#include <memory>
#include <vector>
#include <DirectXMath.h>

#include "Common/D3D12DemoBase.hpp"
#include "Common/ResourceUploadBuffer.hpp"

#include "ConstantBufferDefines.hpp"


class TextureDemo : public D3D12DemoBase {
public:
	TextureDemo (
        uint width,
        uint height,
        std::wstring name
    );

	~TextureDemo();

	void initializeDemo() override;

	void update() override;

	void render (
		ID3D12GraphicsCommandList * drawCmdList
	) override;


private:
	struct Vertex {
		float position[3];
		float normal[3];
	};
	typedef ushort Index;

	std::vector<byte> m_imageData;

	// Constant Buffer specific
	ID3D12DescriptorHeap * m_cbvDescHeap;
	SceneConstants m_sceneConstData[NUM_BUFFERED_FRAMES];
	PointLight m_pointLightConstData[NUM_BUFFERED_FRAMES];

	D3D12_CONSTANT_BUFFER_VIEW_DESC m_cbvDesc_PointLight[NUM_BUFFERED_FRAMES];
	void * m_cbv_PointLight_dataPtr[NUM_BUFFERED_FRAMES];

	D3D12_CONSTANT_BUFFER_VIEW_DESC m_cbvDesc_SceneConstants[NUM_BUFFERED_FRAMES];
	void * m_cbv_SceneConstants_dataPtr[NUM_BUFFERED_FRAMES];

	// Pipeline objects.
	ID3D12RootSignature * m_rootSignature;
	ID3D12PipelineState * m_pipelineState;

	// App resources.
	std::vector<Vertex> m_vertexArray;
	std::vector<ushort> m_indexArray;
	D3D12_INPUT_LAYOUT_DESC m_inputLayoutDesc;
	std::shared_ptr<ResourceUploadBuffer> m_uploadBuffer;

	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
	D3D12_INDEX_BUFFER_VIEW m_indexBufferView;
    uint m_indexCount;

	void loadPipelineDependencies();

	void loadAssets();

	void updateConstantBuffers();

	void createPipelineState (
		ID3DBlob * vertexShaderBlob,
		ID3DBlob * pixelShaderBlob
	);

	void createTextureFromImageFile (
		const char * path,
		ID3D12GraphicsCommandList * uploadCmdList
	);
};

