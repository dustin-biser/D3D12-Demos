#pragma once

#include <memory>
#include "Common/D3D12DemoBase.hpp"
#include "Common/ShaderUtils.hpp"


class IndexRendering : public D3D12DemoBase {
public:
	IndexRendering (
        uint width,
        uint height,
        std::wstring name
    );

	void InitializeDemo (
		ID3D12GraphicsCommandList * uploadCmdList
	) override;

	void Update() override;

	void Render (
		ID3D12GraphicsCommandList * drawCmdList
	) override;

private:
	struct Vertex
	{
		DirectX::XMFLOAT3 position;
		DirectX::XMFLOAT4 color;
	};

	ComPtr<ID3D12RootSignature> m_rootSignature;
	ComPtr<ID3D12PipelineState> m_pipelineState;

	// App resources.
	ComPtr<ID3D12Resource> m_vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
	ComPtr<ID3D12Resource> m_indexBuffer;
	D3D12_INDEX_BUFFER_VIEW m_indexBufferView;
    uint m_indexCount;

	ShaderSource m_vertexShader;
	ShaderSource m_pixelShader;


	void LoadAssets (
		ID3D12GraphicsCommandList * uploadCmdList
	);

    void CreateRootSignature();

    void CreateVertexDataBuffers (ID3D12GraphicsCommandList * uploadCmdList);

    void CreatePipelineState (
		const ShaderSource & vertexShader,
		const ShaderSource & pixelShader
	);


}; // end class IndexRendering
