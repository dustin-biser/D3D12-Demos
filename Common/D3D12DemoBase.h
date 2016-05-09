#pragma once

#include "DemoUtils.hpp"
#include "Win32Application.hpp"

/// Base class for all D3D12 demos.
class D3D12DemoBase
{
public:
	D3D12DemoBase (
        UINT width,
        UINT height,
        std::wstring name
    );

	virtual ~D3D12DemoBase();

	virtual void OnInit() = 0;
	virtual void OnUpdate() = 0;
	virtual void OnRender() = 0;
	virtual void OnDestroy() = 0;

	// Samples override the event handlers to handle specific messages.
	virtual void OnKeyDown(UINT8 /*key*/)   {}
	virtual void OnKeyUp(UINT8 /*key*/)     {}

	// Accessors.
	UINT GetWidth() const           { return m_windowWidth; }
	UINT GetHeight() const          { return m_windowHeight; }
	const WCHAR * GetWindowTitle() const  { return m_title.c_str(); }

	void ParseCommandLineArgs (
        WCHAR * argv[],
        int argc
    );

protected:
    /// Helper function for resolving the full path of assets.
	std::wstring GetAssetPath(LPCWSTR assetName);

    /// Helper function for resolving the full path of assets.
    /// Works with basic multi-byte strings.
	std::string GetAssetPath(const char * assetName);

	void GetHardwareAdapter (
        IDXGIFactory2 * pFactory,
        IDXGIAdapter1 ** ppAdapter
    );

	void SetCustomWindowText (
        LPCWSTR text
    );

	// Viewport dimensions.
	UINT m_windowWidth;
	UINT m_windowHeight;
	float m_aspectRatio;

	// Adapter info.
	bool m_useWarpDevice;

private:
	/// Path of the demo's current working directory.
	std::wstring m_workingDirPath;

	/// The shared solution asset path.
	std::wstring m_sharedAssetPath;

	// Window title.
	std::wstring m_title;
};
