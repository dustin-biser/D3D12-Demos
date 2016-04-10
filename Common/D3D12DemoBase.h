/*
 * D3D12DemoBase.h
 */

#pragma once

#include "DemoUtils.h"
#include "Win32Application.h"

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
	UINT GetWidth() const           { return m_width; }
	UINT GetHeight() const          { return m_height; }
	const WCHAR * GetWindowTitle() const  { return m_title.c_str(); }

	void ParseCommandLineArgs (
        WCHAR * argv[],
        int argc
    );

protected:
	std::wstring GetAssetFullPath(LPCWSTR assetName);

	void GetHardwareAdapter (
        IDXGIFactory2 * pFactory,
        IDXGIAdapter1 ** ppAdapter
    );

	void SetCustomWindowText (
        LPCWSTR text
    );

	// Viewport dimensions.
	UINT m_width;
	UINT m_height;
	float m_aspectRatio;

	// Adapter info.
	bool m_useWarpDevice;

private:
	// Root assets path.
	std::wstring m_assetsPath;

	// Window title.
	std::wstring m_title;
};
