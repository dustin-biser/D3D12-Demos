#pragma once

#include <memory>
#include <vector>
#include <DirectXMath.h>

#include "Common/D3D12DemoBase.hpp"
#include "Common/ImageDecoder.hpp"

#include "ConstantBufferDefines.hpp"


class TextureDemo : public D3D12DemoBase {
public:
	TextureDemo (
        uint windowWidth,
        uint windowHeight,
        std::wstring windowTitle 
    );

	~TextureDemo();

	void InitializeDemo() override;

	void Update() override;

	void Render (
		ID3D12GraphicsCommandList * drawCmdList
	) override;


private:
	struct Vertex {
		float position[3];
		float normal[3];
		float texCoord[2];
	};
	typedef ushort Index;

	ImageData m_imageData;
	ComPtr<ID3D12Resource> m_imageTexture2d;
	ComPtr<ID3D12Resource> m_uploadBuffer;

	ComPtr<ID3D12DescriptorHeap> m_srvDescriptorHeap;


	// Constant Buffer specific
	SceneConstants m_sceneConstData[NUM_BUFFERED_FRAMES];
	ComPtr<ID3D12Resource> m_constantBuffer_sceneConstant[NUM_BUFFERED_FRAMES];
	DirectionalLight m_pointLightConstData[NUM_BUFFERED_FRAMES];
	ComPtr<ID3D12Resource> m_constantBuffer_pointLight[NUM_BUFFERED_FRAMES];

	// Pipeline objects.
	ComPtr<ID3D12RootSignature> m_rootSignature;
	ComPtr<ID3D12PipelineState> m_pipelineState;

	// App resources.
	uint m_numIndices;
	D3D12_INPUT_LAYOUT_DESC m_inputLayoutDesc;
	ComPtr<ID3D12Resource> m_vertexBuffer;
	ComPtr<ID3D12Resource> m_indexBuffer;

	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
	D3D12_INDEX_BUFFER_VIEW m_indexBufferView;
    uint m_indexCount;

	void UploadVertexDataToGpu();

	void CreateRootSignature();

	void CreateConstantBuffers();

	void UpdateConstantBuffers();

	void createPipelineState (
		ID3DBlob * vertexShaderBlob,
		ID3DBlob * pixelShaderBlob
	);

	void CreateTexture();

	void CreateDescriptorHeap();
};

