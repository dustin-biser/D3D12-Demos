#pragma once

#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_4.h>

#include "Common/NumericTypes.hpp"
#include "Common/DemoUtils.hpp"
#include "Common/Win32Application.hpp"

/// Base class for all D3D12 demos.
class D3D12DemoBase
{
public:
	D3D12DemoBase (
        uint windowWidth,
        uint windowHeight,
        std::wstring windowTitle
    );

	virtual ~D3D12DemoBase();


	virtual void OnKeyDown (
		uint8 key
	);
	virtual void OnKeyUp (
		uint8 key
	);

	void Initialize();

	void BuildNextFrame();

	void PresentNextFrame();

	void PrepareCleanup();

	uint GetWindowWidth() const;

	uint GetWindowHeight() const;

	const WCHAR * GetWindowTitle() const;


protected:
	template <typename T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	bool m_vsyncEnabled = true;

	// Number of rendered frames to pre-flight for execution on the GPU.
	static const uint NUM_BUFFERED_FRAMES = 3;
	uint m_frameIndex;

	// Window and viewport dimensions.
	uint m_windowWidth;
	uint m_windowHeight;
	D3D12_VIEWPORT m_viewport;
	D3D12_RECT m_scissorRect;

	// Set to true to use software rasterizer.
	bool m_useWarpDevice;

	ComPtr<ID3D12Device> m_device;

	// Direct Command Queue Related
	ComPtr<ID3D12CommandQueue> m_directCmdQueue;
	ComPtr<ID3D12CommandAllocator> m_directCmdAllocator[NUM_BUFFERED_FRAMES];
	ComPtr<ID3D12GraphicsCommandList> m_drawCmdList[NUM_BUFFERED_FRAMES];

	// Upload Command List.
	// Used for uploading data to GPU during demo initialization.
	ComPtr<ID3D12CommandAllocator> m_uploadCmdAllocator;
	ComPtr<ID3D12GraphicsCommandList> m_uploadCmdList;

	ComPtr<IDXGISwapChain3> m_swapChain;
	HANDLE m_frameLatencyWaitableObject;

	// Depth/Stencil specific
	ComPtr<ID3D12DescriptorHeap> m_dsvDescHeap;
	ComPtr<ID3D12Resource> m_depthStencilBuffer;

	struct RenderTarget {
		// Handle to render target view within descriptor heap.
		D3D12_CPU_DESCRIPTOR_HANDLE descriptorHandle;
		ID3D12Resource * resource;

		~RenderTarget() { resource->Release(); }
	};
	RenderTarget m_renderTarget[NUM_BUFFERED_FRAMES];

	// Render Target View descriptor heap
	ComPtr<ID3D12DescriptorHeap> m_rtvDescHeap;


	// Synchronization objects.
	HANDLE m_frameFenceEvent[NUM_BUFFERED_FRAMES];
	ID3D12Fence * m_frameFence[NUM_BUFFERED_FRAMES];
	uint64 m_currentFenceValue;
	uint64 m_fenceValue[NUM_BUFFERED_FRAMES];


	virtual void InitializeDemo() = 0;

	virtual void Update() = 0;

	virtual void Render (
		ID3D12GraphicsCommandList * drawCmdList
	) = 0;

	void PrepareRender (
		ID3D12CommandAllocator * commandAllocator,
		ID3D12GraphicsCommandList * drawCmdList
	);

	void FinalizeRender (
		ID3D12GraphicsCommandList * drawCmdList,
		ID3D12CommandQueue * commandQueue
	);

	void Present();

	__forceinline bool SwapChainWaitableObjectIsSignaled();

	// Issues a Signal from 'commanQueue', and causes current thread to block
	// until GPU completes the Signal.
	void WaitForGpuCompletion (
		ID3D12CommandQueue * commandQueue
	);


	/// Helper function for resolving the full path of assets.
	std::wstring GetAssetPath (
		const wchar_t * assetName
	);

	/// Helper function for resolving the full path of assets.
	/// Works with basic multi-byte strings.
	std::string GetAssetPath (
		const char * assetName
	);

	void SetCustomWindowText(
		LPCWSTR text
	);

private:
	/// Path of the demo's current working directory.
	std::wstring m_workingDirPath;

	/// The shared solution asset path.
	std::wstring m_sharedAssetPath;

	// Window title.
	std::wstring m_windowTitle;

	void CreateDirectCommandQueue ();

	void CreateDrawCommandLists ();

	void CreateCopyCommandList ();

	void CreateDepthStencilBuffer ();

	void CreateDeviceAndSwapChain ();

	void CreateHardwareDevice (
		IDXGIFactory4 * dxgiFactory,
		D3D_FEATURE_LEVEL featureLevel
	);

};

// Causes current thread to wait on 'fenceEvent' until GPU fence value
// is equal to 'completionValue'.
void WaitForGpuFence (
	ID3D12Fence * fence,
	uint64 completionValue,
	HANDLE fenceEvent
);
