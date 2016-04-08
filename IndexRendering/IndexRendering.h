// IndexRendering.h

#pragma once

#include "DXSample.h"
#include "Fence.h"
#include <memory>

using namespace DirectX;

// Note that while ComPtr is used to manage the lifetime of resources on the CPU,
// it has no understanding of the lifetime of resources on the GPU. Apps must account
// for the GPU lifetime of resources to avoid destroying objects that may still be
// referenced by the GPU.
// An example of this can be found in the class method: OnDestroy().
using Microsoft::WRL::ComPtr;


class IndexRendering : public DXSample {
public:
	IndexRendering (
        uint width,
        uint height,
        std::wstring name
    );

	virtual void OnInit();
	virtual void OnUpdate();
	virtual void OnRender();
	virtual void OnDestroy();

private:
    /// Number of buffered frames
	static const uint FrameCount = 2;

	struct Vertex
	{
		XMFLOAT3 position;
		XMFLOAT4 color;
	};

	// Pipeline objects.
	D3D12_VIEWPORT m_viewport;
	D3D12_RECT m_scissorRect;
	ComPtr<IDXGISwapChain3> m_swapChain;
	ComPtr<ID3D12Device> m_device;
	ComPtr<ID3D12CommandAllocator> m_commandAllocator;
	ComPtr<ID3D12CommandQueue> m_commandQueue;
	ComPtr<ID3D12RootSignature> m_rootSignature;
	ComPtr<ID3D12PipelineState> m_pipelineState;
	ComPtr<ID3D12GraphicsCommandList> m_drawCommandList[FrameCount];
    ComPtr<ID3D12GraphicsCommandList> m_copyCommandList;

	ComPtr<ID3D12Resource> m_renderTargets[FrameCount];
	ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
	uint m_rtvDescriptorSize;

	// App resources.
	ComPtr<ID3D12Resource> m_vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
	ComPtr<ID3D12Resource> m_indexBuffer;
	D3D12_INDEX_BUFFER_VIEW m_indexBufferView;
    uint m_indexCount;

	// Synchronization objects.
	uint m_frameIndex;
	std::shared_ptr<Fence> m_fence;

	void LoadRenderPipelineDependencies();
	void LoadAssets();
	void PopulateCommandList();
	void WaitForPreviousFrame();

    void CreateDevice (
        _In_ IDXGIFactory4 * dxgiFactory,
        _In_ bool useWarpDevice,
        _Out_ ComPtr<ID3D12Device> & device
    );

    void CreateCommandQueue (
        _In_ ID3D12Device * device,
        _Out_ ComPtr<ID3D12CommandQueue> & commandQueue
    );

    void CreateSwapChain (
        _In_ IDXGIFactory4 * dxgiFactory,
        _In_ ID3D12CommandQueue * commandQueue,
        _In_ uint framebufferWidth,
        _In_ uint framebufferHeight,
        _Out_ ComPtr<IDXGISwapChain3> & swapChain
    );

    void CreateRenderTargetView (
        _In_ ID3D12Device * device,
        _In_ IDXGISwapChain * swapChain,
        _Out_ ComPtr<ID3D12DescriptorHeap> & rtvHeap,
        _Out_ uint & rtvDescriptorSize,
        _Out_ ComPtr<ID3D12Resource> * renderTargets
    );

    void CreateRootSignature (
        _In_ ID3D12Device * device,
        _Out_ ComPtr<ID3D12RootSignature> & rootSignature
    );

    void LoadShaders (
        _Out_ ComPtr<ID3DBlob> & vertexShaderBlob,
        _Out_ ComPtr<ID3DBlob> & pixelShaderBlob
    );

    void CreatePipelineState (
        _In_ ID3D12Device * device,
        _In_ ID3D12RootSignature * rootSignature,
        _In_ ID3DBlob * vertexShaderBlob,
        _In_ ID3DBlob * pixelShaderBlob,
        _Out_ ComPtr<ID3D12PipelineState> & pipelineState
    );

    void CreateDrawCommandLists (
        _In_ ID3D12Device * device,
        _In_ ID3D12CommandAllocator * commandAllocator,
        _In_ uint numDrawCommandLists,
        _Out_ ComPtr<ID3D12GraphicsCommandList> * drawCommandList
    );

    D3D12_VERTEX_BUFFER_VIEW UploadVertexDataToDefaultHeap (
        _In_ ID3D12Device * device,
        _In_ ID3D12GraphicsCommandList * copyCommandList,
        _Out_ ComPtr<ID3D12Resource> & vertexBufferUploadHeap,
        _Out_ ComPtr<ID3D12Resource> & vertexBuffer
    );

    D3D12_INDEX_BUFFER_VIEW UploadIndexDataToDefaultHeap (
        _In_ ID3D12Device * device,
        _In_ ID3D12GraphicsCommandList * copyCommandList,
        _Out_ ComPtr<ID3D12Resource> & indexBufferUploadHeap,
        _Out_ ComPtr<ID3D12Resource> & indexBuffer,
        _Out_ uint & indexCount
    );

    void CreateVertexDataBuffers (
        _In_ ID3D12Device * device,
        _In_ ID3D12CommandQueue * commandQueue,
        _In_ ID3D12GraphicsCommandList * copyCommandList,
        _Out_ ComPtr<ID3D12Resource> & vertexBuffer,
        _Out_ ComPtr<ID3D12Resource> & indexBuffer,
        _Out_ uint & indexCount
    );


}; // end class IndexRendering
