#pragma once

#include <memory>
#include "Common/D3D12DemoBase.hpp"


class IndexRendering : public D3D12DemoBase {
public:
	IndexRendering (
        uint width,
        uint height,
        std::wstring name
    );

	void initializeDemo() override;
	void update() override;
	void render() override;
	void cleanupDemo() override;

private:
	struct Vertex
	{
		DirectX::XMFLOAT3 position;
		DirectX::XMFLOAT4 color;
	};

	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipelineState;

	// App resources.
	Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_indexBuffer;
	D3D12_INDEX_BUFFER_VIEW m_indexBufferView;
    uint m_indexCount;

	void loadAssets();
	void populateCommandList();
    void createRootSignature();
    void createVertexDataBuffers();
    void loadShaders (
        _Out_ Microsoft::WRL::ComPtr<ID3DBlob> & vertexShaderBlob,
        _Out_ Microsoft::WRL::ComPtr<ID3DBlob> & pixelShaderBlob
    );
    void createPipelineState (
        _In_ ID3DBlob * vertexShaderBlob,
        _In_ ID3DBlob * pixelShaderBlob
    );


}; // end class IndexRendering
