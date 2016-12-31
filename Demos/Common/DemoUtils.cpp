/*
 * DemoUtils.cpp
 */

#include "pch.h"
using Microsoft::WRL::ComPtr;

#include "DemoUtils.hpp"

#include <cstdlib>
#include <cwchar>
#include <iostream>
#include <vector>

//---------------------------------------------------------------------------------------
void QueryVideoMemoryInfo (
    _In_ ID3D12Device * device,
	_In_ DXGI_MEMORY_SEGMENT_GROUP memoryGroup,
	_Out_ DXGI_QUERY_VIDEO_MEMORY_INFO & videoMemoryInfo
) {
    assert(device);

    ComPtr<IDXGIFactory4> dxgiFactory;
    CHECK_D3D_RESULT(
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
std::wstring toWString (const std::string & str)
{
	const size_t STRING_LENGTH = str.length ();
	std::vector<wchar_t> destBuffer (STRING_LENGTH + 1);

	// Convert multi-byte string to wide character string.
	mbstowcs (destBuffer.data (), str.data (), STRING_LENGTH);
	destBuffer[STRING_LENGTH] = L'\0';

	return std::wstring(destBuffer.data());
}

//---------------------------------------------------------------------------------------
void GetWorkingDir (
	_Out_writes_(pathSize) char * path,
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

	char * lastSlash = strrchr(path, '\\');
	if (lastSlash)
	{
		*(lastSlash + 1) = '\0';
	}
}

//---------------------------------------------------------------------------------------
void GetSolutionDir (
	_Out_writes_(pathSize) char * path,
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

    char * lastMatch = strstr(path, "\\Demos");
	if (lastMatch)
	{
		*(lastMatch) = '\0';
	}
}
