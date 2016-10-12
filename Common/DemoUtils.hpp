#pragma once

#include <cassert>
#include <cwchar>
#include <iostream>


#if defined(_DEBUG)
#define CHECK_D3D_RESULT(x)									 \
	do {													 \
		HRESULT result = (x);								 \
		if ( FAILED(result) ) {								 \
			std::cout << "Direct3D Error at " << __FILE__ << \
				":" << __LINE__ << std::endl;				 \
			assert(result == S_OK);							 \
		}													 \
	}														 \
	while(0)
#else
#define CHECK_D3D_RESULT(x) x;
#endif


#if defined(_DEBUG)
	// Assigns a default name to a single D3D12 object to aid in identification
	// of the object during graphics debugging.
	// @param x - pointer to a D3D12 object type.
#define NAME_D3D12_OBJECT(x) \
	x->SetName(L#x);
#else
#define NAME_D3D12_OBJECT(x)
#endif


#if defined(_DEBUG)
	// Assigns default names to an array of D3D12 objects to aid in identification
	// of the objects during graphics debugging.
	// @param x - array of ComPtr<T> where T is a D3D12 object type.
	// @param n - size of array.
#define NAME_D3D12_OBJECT_ARRAY(x, n) \
	do { \
		for (unsigned int i(0); i < (n); ++i) { \
			WCHAR buffer[256]; \
			wsprintf(buffer, L"%ls[%u]", L#x, i); \
			x[i]->SetName(buffer); \
		} \
	} while(0)
#else
#define NAME_D3D12_OBJECT_ARRAY(x, n)
#endif


#if defined(_DEBUG)
	// Sets a specific name for a single D3D12 object to aid in identification
	// of the object during graphics debugging.
	// @param x - ComPtr<T> where T is a D3D12 object type.
#define D3D12_SET_NAME(x, name) \
	x->SetName(name);
#else
#define D3D12_SET_NAME(x, name)
#endif


void GetWorkingDir (
	_Out_writes_(pathSize) WCHAR* path,
	UINT pathSize
);


void GetSolutionDir (
	_Out_writes_(pathSize) WCHAR* path,
	UINT pathSize
);


/// Query a D3D12Device to determine memory usage information.
void QueryVideoMemoryInfo (
	_In_ ID3D12Device * device,
	_In_ DXGI_MEMORY_SEGMENT_GROUP memoryGroup,
	_Out_ DXGI_QUERY_VIDEO_MEMORY_INFO & videoMemoryInfo
);

template <typename T>
constexpr T RoundToNextMultiple (const T a, const T multiple)
{
	return ((a + multiple - 1) / multiple) * multiple;
}


// Force runtime to break with optional message.
#define ForceBreak(message) \
	do { \
		if (message) { \
			std::wcout << (message) << std::endl; \
		} \
		__debugbreak(); \
	} while(0)
