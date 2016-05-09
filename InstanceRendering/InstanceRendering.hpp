#pragma once

#include <memory>
#include <vector>
#include <DirectXMath.h>

#include "D3D12DemoBase.h"
using Microsoft::WRL::ComPtr;

#include "ResourceUploadBuffer.hpp"
#include "ConstantBufferDefines.hpp"


//--Forward Declarations:
class Fence;


class InstanceRendering : public D3D12DemoBase {
public:
	InstanceRendering (
        uint width,
        uint height,
        std::wstring name
    );

	virtual void OnInit();
	virtual void OnUpdate();
	virtual void OnRender();
	virtual void OnDestroy();
	

private:
	struct Vertex {
		float position[3];
		float normal[3];
	};
	typedef ushort Index;

    // Number of buffered frames to pre-flight on the GPU.
	static const uint NumBufferedFrames = 3;
	uint m_frameIndex;

	// Constant Buffer specific
	ComPtr<ID3D12DescriptorHeap> m_cbvDescHeap;
	SceneConstants m_sceneConstData[NumBufferedFrames];
	PointLight m_pointLightConstData[NumBufferedFrames];

	D3D12_CONSTANT_BUFFER_VIEW_DESC m_cbvDesc_PointLight[NumBufferedFrames];
	void * m_cbv_PointLight_dataPtr[NumBufferedFrames];

	D3D12_CONSTANT_BUFFER_VIEW_DESC m_cbvDesc_SceneConstants[NumBufferedFrames];
	void * m_cbv_SceneConstants_dataPtr[NumBufferedFrames];

	// Pipeline objects.
	D3D12_VIEWPORT m_viewport;
	D3D12_RECT m_scissorRect;
	ComPtr<IDXGISwapChain3> m_swapChain;
	ComPtr<ID3D12Device> m_device;
	ComPtr<ID3D12CommandAllocator> m_cmdAllocator;
	ComPtr<ID3D12CommandQueue> m_commandQueue;
	ComPtr<ID3D12RootSignature> m_rootSignature;
	ComPtr<ID3D12PipelineState> m_pipelineState;
	ComPtr<ID3D12GraphicsCommandList> m_drawCmdList[NumBufferedFrames];

	// Render Target specific
	ComPtr<ID3D12Resource> m_renderTarget[NumBufferedFrames];
	ComPtr<ID3D12DescriptorHeap> m_rtvDescHeap;

	// Depth/Stencil specific
	ComPtr<ID3D12DescriptorHeap> m_dsvDescHeap;
	ComPtr<ID3D12Resource> m_depthStencilBuffer;

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

	// Synchronization objects.
	std::shared_ptr<Fence> m_fence;

	void LoadPipelineDependencies();
	void LoadAssets();
	void PopulateCommandList();
	void UpdateConstantBuffers();
	void WaitForGPUSync();

}; // end class FrameBuffering
