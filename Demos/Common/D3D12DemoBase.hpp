#pragma once

#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_4.h>

#include "Common/BasicTypes.hpp"
#include "Common/DemoUtils.hpp"
#include "Common/Win32Application.hpp"



struct ScreenPosition {
	uint x;
	uint y;
};



/// Base class for all D3D12 demos.
class D3D12DemoBase
{
public:
	D3D12DemoBase (
        uint windowWidth,
        uint windowHeight,
        std::string windowTitle
    );

	virtual ~D3D12DemoBase();


	virtual void OnKeyDown (
		uint8 key
	);

	virtual void OnKeyUp (
		uint8 key
	);

	virtual void OnResize (
		uint windowWidth,
		uint windowHeight
	);

	virtual void OnMouseMove (
		int dx,
		int dy
	);

	void UpdateMousePosition (
		uint x,
		uint y
	);
	
	void MouseLButtonDown();

	void MouseLButtonUp();

	void Initialize();

	void BuildNextFrame();

	void PresentNextFrame();

	void PrepareCleanup();

	uint GetWindowWidth() const;

	uint GetWindowHeight() const;

	const char * GetWindowTitle() const;

	ScreenPosition GetMousePosition() const;


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

	ScreenPosition m_mousePosition;
	bool m_mouseLButtonDown;


	ComPtr<ID3D12Device> m_device;

	// Direct Command Queue Related
	ComPtr<ID3D12CommandQueue> m_directCmdQueue;
	ComPtr<ID3D12CommandAllocator> m_directCmdAllocator[NUM_BUFFERED_FRAMES];
	ComPtr<ID3D12GraphicsCommandList> m_drawCmdList[NUM_BUFFERED_FRAMES];


	ComPtr<IDXGISwapChain3> m_swapChain;
	HANDLE m_frameLatencyWaitableObject;

	// Resources that are referenced by descriptor handles (a.k.a. resource views).
	struct HandledResource {
		union {
			D3D12_CPU_DESCRIPTOR_HANDLE descHandle;
			D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle;
			D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle;
		};

		ID3D12Resource * resource;

		~HandledResource() { resource->Release(); }
	};

	// Depth-Stencil Resource 
	ComPtr<ID3D12DescriptorHeap> m_dsvDescHeap;
	HandledResource m_depthStencilBuffer;

	// Render Target Resources
	ComPtr<ID3D12DescriptorHeap> m_rtvDescHeap;
	HandledResource m_renderTarget[NUM_BUFFERED_FRAMES];


	// Synchronization objects.
	HANDLE m_frameFenceEvent[NUM_BUFFERED_FRAMES];
	ComPtr<ID3D12Fence> m_frameFence[NUM_BUFFERED_FRAMES];
	uint64 m_currentFenceValue;
	uint64 m_fenceValue[NUM_BUFFERED_FRAMES];


	virtual void InitializeDemo (
		ID3D12GraphicsCommandList * uploadCmdList
	) = 0;

	virtual void Update () = 0;

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
	std::string GetAssetPath (
		const char * assetName
	);

	void SetCustomWindowText (
		LPCSTR text
	);

private:
	/// Path of the demo's current working directory.
	std::string m_workingDirPath;

	/// The shared solution asset path.
	std::string m_sharedAssetPath;

	// Window title.
	std::string m_windowTitle;

	void CreateDirectCommandQueue ();

	void CreateDrawCommandLists ();

	void GenerateCommandList (
		ComPtr<ID3D12GraphicsCommandList> & commandList,
		ComPtr<ID3D12CommandAllocator> & cmdAllocator
	);

	void CreateDepthStencilBuffer ();

	void CreateDeviceAndSwapChain ();

	void CreateHardwareDevice (
		ID3D12Device ** deviceToCreate,
		IDXGIFactory4 * dxgiFactory,
		D3D_FEATURE_LEVEL featureLevel
	);

	void CreateFenceObjects();

	void CreateRenderTargetViews();

};

// Causes current thread to wait on 'fenceEvent' until GPU fence value
// is equal to 'completionValue'.
void WaitForGpuFence (
	ID3D12Fence * fence,
	uint64 completionValue,
	HANDLE fenceEvent
);
