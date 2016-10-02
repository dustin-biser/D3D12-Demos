#pragma once

#include <memory>
#include "D3D12DemoBase.hpp"


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

	// Pipeline objects.
	D3D12_VIEWPORT m_viewport;
	D3D12_RECT m_scissorRect;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_commandAllocator[NUM_BUFFERED_FRAMES];
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_drawCommandList[NUM_BUFFERED_FRAMES];
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_copyCommandAllocator;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_copyCommandList;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipelineState;

	Microsoft::WRL::ComPtr<ID3D12Resource> m_renderTargets[NUM_BUFFERED_FRAMES];
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
	uint m_rtvDescriptorSize;

	// App resources.
	Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_indexBuffer;
	D3D12_INDEX_BUFFER_VIEW m_indexBufferView;
    uint m_indexCount;


	void loadRenderPipelineDependencies();
	void loadAssets();
	void populateCommandList();

    void createRenderTargetView (
        _In_ ID3D12Device * device,
        _In_ IDXGISwapChain * swapChain,
        _Out_ Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> & rtvHeap,
        _Out_ uint & rtvDescriptorSize,
        _Out_ Microsoft::WRL::ComPtr<ID3D12Resource> * renderTargets
    );

    void createRootSignature (
        _In_ ID3D12Device * device,
        _Out_ Microsoft::WRL::ComPtr<ID3D12RootSignature> & rootSignature
    );

    void loadShaders (
        _Out_ Microsoft::WRL::ComPtr<ID3DBlob> & vertexShaderBlob,
        _Out_ Microsoft::WRL::ComPtr<ID3DBlob> & pixelShaderBlob
    );

    void createPipelineState (
        _In_ ID3D12Device * device,
        _In_ ID3D12RootSignature * rootSignature,
        _In_ ID3DBlob * vertexShaderBlob,
        _In_ ID3DBlob * pixelShaderBlob,
        _Out_ Microsoft::WRL::ComPtr<ID3D12PipelineState> & pipelineState
    );

    void createVertexDataBuffers();


}; // end class IndexRendering
