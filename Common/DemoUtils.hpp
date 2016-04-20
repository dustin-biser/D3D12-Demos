/*
 * DemoUtils.h
 */

#pragma once

inline void ThrowIfFailed(HRESULT hr)
{
	if (FAILED(hr))
	{
		throw;
	}
}

#if defined(_DEBUG)
    // Assign a name to the object to aid with debugging.
    inline void SetName(ID3D12Object* pObject, LPCWSTR name)
    {
        pObject->SetName(name);
    }
#else
    inline void SetName(ID3D12Object*, LPCWSTR)
    {
    }
#endif

#if defined(_DEBUG)
    // Naming helper for ComPtr<T>.
    // Assigns the name of the variable as the name of the object.
    #define NAME_D3D12_OBJECT(x) SetName(x.Get(), L#x)
#else
    #define NAME_D3D12_OBJECT(x)
#endif


void GetWorkingDir(
	_Out_writes_(pathSize) WCHAR* path,
	UINT pathSize
);


void GetSolutionDir(
	_Out_writes_(pathSize) WCHAR* path,
	UINT pathSize
);

/**
 * Query a D3D12Device to determine memory usage information.
 */
void QueryVideoMemoryInfo(
	_In_ ID3D12Device * device,
	_In_ DXGI_MEMORY_SEGMENT_GROUP memoryGroup,
	_Out_ DXGI_QUERY_VIDEO_MEMORY_INFO & videoMemoryInfo
);
