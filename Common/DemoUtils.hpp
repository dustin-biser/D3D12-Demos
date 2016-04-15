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

inline void GetWorkingDir(_Out_writes_(pathSize) WCHAR* path, UINT pathSize)
{
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

inline void GetSolutionDir(_Out_writes_(pathSize) WCHAR* path, UINT pathSize)
{
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
