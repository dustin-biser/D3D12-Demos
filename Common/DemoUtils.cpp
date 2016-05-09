/*
 * DemoUtils.cpp
 */

#include "pch.h"
using Microsoft::WRL::ComPtr;

#include "DemoUtils.hpp"

#include <dxgi1_4.h>
#include <d3dx12.h>

//---------------------------------------------------------------------------------------
void QueryVideoMemoryInfo (
    _In_ ID3D12Device * device,
	_In_ DXGI_MEMORY_SEGMENT_GROUP memoryGroup,
	_Out_ DXGI_QUERY_VIDEO_MEMORY_INFO & videoMemoryInfo
) {
    assert(device);

    ComPtr<IDXGIFactory4> dxgiFactory;
    CHECK_DX_RESULT(
        CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory))
    );

    LUID adapterLUID = device->GetAdapterLuid();
    
    IDXGIAdapter3 * pDXGIAdapter3;
    dxgiFactory->EnumAdapterByLuid(adapterLUID, IID_PPV_ARGS(&pDXGIAdapter3));

    pDXGIAdapter3->QueryVideoMemoryInfo ( // Query GPU memory info
        0, memoryGroup, &videoMemoryInfo
    );
}

//---------------------------------------------------------------------------------------
void GetWorkingDir (
	_Out_writes_(pathSize) WCHAR* path,
	UINT pathSize
) {
	if (path == nullptr)
	{
		throw;
	}

	DWORD size = GetModuleFileName(nullptr, path, pathSize);
	if (size == 0 || size == pathSize)
	{
		// Method failed or path was truncated.
		throw;
	}

	WCHAR* lastSlash = wcsrchr(path, L'\\');
	if (lastSlash)
	{
		*(lastSlash + 1) = L'\0';
	}
}

//---------------------------------------------------------------------------------------
void GetSolutionDir (
	_Out_writes_(pathSize) WCHAR* path,
	UINT pathSize
) {
	if (path == nullptr)
	{
		throw;
	}

	DWORD size = GetModuleFileName(nullptr, path, pathSize);
	if (size == 0 || size == pathSize)
	{
		// Method failed or path was truncated.
		throw;
	}

    WCHAR* lastMatch = wcsstr(path, L"\\bin");
	if (lastMatch)
	{
		*(lastMatch) = L'\0';
	}
    lastMatch = wcsrchr(path, L'\\');
	if (lastMatch)
	{
		*(lastMatch + 1) = L'\0';
	}
}
