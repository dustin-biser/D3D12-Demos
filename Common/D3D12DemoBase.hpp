//
// D3D12DemoBase.hpp
//
#pragma once

#include <sal.h>
#include <wrl.h>

#include <d3d12.h>
#include <dxgi1_4.h>
#include "NumericTypes.hpp"
#include "DemoUtils.hpp"
#include "Win32Application.hpp"

/// Base class for all D3D12 demos.
class D3D12DemoBase
{
public:
	D3D12DemoBase (
        uint windowWidth,
        uint windowHeight,
        std::wstring name
    );

	virtual ~D3D12DemoBase();

	virtual void initializeDemo();
	virtual void update() = 0;
	virtual void render() = 0;
	virtual void cleanupDemo();

	virtual void onKeyDown(uint8 key);
	virtual void onKeyUp(uint8 key);

	void buildNextFrame();
	void presentNextFrame();

	uint getWindowWidth() const;

	uint getWindowHeight() const;

	const WCHAR * getWindowTitle() const;


protected:
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

	Microsoft::WRL::ComPtr<IDXGIFactory4> m_dxgiFactory;
	Microsoft::WRL::ComPtr<ID3D12Device> m_device;

	// Direct Command Queue Related
	ID3D12CommandQueue * m_directCmdQueue;
	ID3D12CommandAllocator * m_directCmdAllocator[NUM_BUFFERED_FRAMES];
	ID3D12GraphicsCommandList * m_drawCmdList[NUM_BUFFERED_FRAMES];

	// Copy Command Related
	ID3D12CommandAllocator * m_copyCmdAllocator;
	ID3D12GraphicsCommandList * m_copyCmdList;

	// SwapChain objects.
	Microsoft::WRL::ComPtr<IDXGISwapChain3> m_swapChain;
	HANDLE m_frameLatencyWaitableObject;

	struct RenderTarget {
		ID3D12Resource * resource;
		// Handle to render target view within descriptor heap.
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle;
	};
	RenderTarget m_renderTarget[NUM_BUFFERED_FRAMES];

	struct DescriptorHeap {
		// Size of handle increment for next descriptor within heap.
		uint handleIncrementSize;
		ID3D12DescriptorHeap * pHeap;
	};
	DescriptorHeap m_rtvDescHeap;


	// Synchronization objects.
	HANDLE m_frameFenceEvent[NUM_BUFFERED_FRAMES];
	Microsoft::WRL::ComPtr<ID3D12Fence> m_frameFence[NUM_BUFFERED_FRAMES];
	uint64 m_currentFenceValue;
	uint64 m_fenceValue[NUM_BUFFERED_FRAMES];


	void present();

	__forceinline bool swapChainWaitableObjectIsSignaled();


	// Issues a Signal from 'commanQueue', and causes current thread to block
	// until GPU completes the Signal.
	void waitForGpuCompletion(ID3D12CommandQueue * commandQueue);


	/// Helper function for resolving the full path of assets.
	std::wstring getAssetPath(LPCWSTR assetName);

	/// Helper function for resolving the full path of assets.
	/// Works with basic multi-byte strings.
	std::string getAssetPath(const char * assetName);

	void setCustomWindowText(
		_In_ LPCWSTR text
	);

private:
	/// Path of the demo's current working directory.
	std::wstring m_workingDirPath;

	/// The shared solution asset path.
	std::wstring m_sharedAssetPath;

	// Window title.
	std::wstring m_title;

	void createDrawCommandLists();
	void createCopyCommandList();
};


// Causes current thread to wait on 'fenceEvent' until GPU fence value
// is equal to 'completionValue'.
void waitForFrameFence (
	_In_ ID3D12Fence * fence,
	_In_ uint64 completionValue,
	_In_ HANDLE fenceEvent
);
