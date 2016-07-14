#pragma once

#include <memory>
#include <vector>
#include <DirectXMath.h>

#include "D3D12DemoBase.hpp"
#include "ResourceUploadBuffer.hpp"
#include "ConstantBufferDefines.hpp"


class ConstantBufferDemo : public D3D12DemoBase {
public:
	ConstantBufferDemo (
        uint width,
        uint height,
        std::wstring name
    );

	virtual void initializeDemo();
	virtual void update();
	virtual void render();
	virtual void cleanupDemo();
	

private:
	struct Vertex {
		float position[3];
		float normal[3];
	};
	typedef ushort Index;

	// Constant Buffer specific
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_cbvDescHeap;
	SceneConstants m_sceneConstData[NUM_BUFFERED_FRAMES];
	PointLight m_pointLightConstData[NUM_BUFFERED_FRAMES];

	D3D12_CONSTANT_BUFFER_VIEW_DESC m_cbvDesc_PointLight[NUM_BUFFERED_FRAMES];
	void * m_cbv_PointLight_dataPtr[NUM_BUFFERED_FRAMES];

	D3D12_CONSTANT_BUFFER_VIEW_DESC m_cbvDesc_SceneConstants[NUM_BUFFERED_FRAMES];
	void * m_cbv_SceneConstants_dataPtr[NUM_BUFFERED_FRAMES];

	// Pipeline objects.
	D3D12_VIEWPORT m_viewport;
	D3D12_RECT m_scissorRect;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipelineState;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_cmdAllocator[NUM_BUFFERED_FRAMES];
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_drawCmdList[NUM_BUFFERED_FRAMES];

	// Render Target specific
	Microsoft::WRL::ComPtr<ID3D12Resource> m_renderTarget[NUM_BUFFERED_FRAMES];
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_rtvDescHeap;

	// Depth/Stencil specific
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_dsvDescHeap;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_depthStencilBuffer;

	// App resources.
	std::vector<Vertex> m_vertexArray;
	std::vector<ushort> m_indexArray;
	D3D12_INPUT_LAYOUT_DESC m_inputLayoutDesc;
	std::shared_ptr<ResourceUploadBuffer> m_uploadBuffer;

	Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_indexBuffer;
	D3D12_INDEX_BUFFER_VIEW m_indexBufferView;
    uint m_indexCount;

	void loadPipelineDependencies();

	void loadAssets();

	void populateCommandList();

	void updateConstantBuffers();

}; // end class FrameBuffering
