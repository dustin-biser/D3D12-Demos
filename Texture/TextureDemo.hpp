#pragma once

#include <memory>
#include <vector>
#include <DirectXMath.h>

#include "Common/D3D12DemoBase.hpp"

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
		float texCoord[2];
	};
	typedef ushort Index;

	std::vector<byte> m_imageData;

	// Constant Buffer specific
	SceneConstants m_sceneConstData[NUM_BUFFERED_FRAMES];
	ID3D12Resource * m_constantBuffer_sceneConstant[NUM_BUFFERED_FRAMES];
	DirectionalLight m_pointLightConstData[NUM_BUFFERED_FRAMES];
	ID3D12Resource * m_constantBuffer_pointLight[NUM_BUFFERED_FRAMES];

	// Pipeline objects.
	ID3D12RootSignature * m_rootSignature;
	ID3D12PipelineState * m_pipelineState;

	// App resources.
	uint m_numIndices;
	D3D12_INPUT_LAYOUT_DESC m_inputLayoutDesc;
	ID3D12Resource * m_vertexBuffer;
	ID3D12Resource * m_indexBuffer;

	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
	D3D12_INDEX_BUFFER_VIEW m_indexBufferView;
    uint m_indexCount;

	void uploadVertexDataToGpu();

	void createRootSignature();

	void createConstantBuffers();

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

