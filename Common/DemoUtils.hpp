#pragma once

#include <cassert>
#include <cwchar>
#include <iostream>
#include <debugapi.h>

#define LOG_BUFFER_LENGTH 256
#define LOG_LEVEL_INFO L"Log Info: "
#define LOG_LEVEL_WARNING L"Log Warning: "
#define LOG_LEVEL_ERROR L"Log Error: "

#if defined(_DEBUG)
#define LOG(levelWString, format, ...) \
	do { \
		wchar_t buffer[LOG_BUFFER_LENGTH]; \
		int wcharsWritten = swprintf(buffer, LOG_BUFFER_LENGTH, levelWString); \
		wcharsWritten += swprintf(buffer + wcharsWritten, LOG_BUFFER_LENGTH, L##format, __VA_ARGS__); \
		swprintf(buffer + wcharsWritten, LOG_BUFFER_LENGTH, L"\n"); \
		OutputDebugStringW(buffer); \
	} while(0)
#else
#define LOG(levelWString, format, ...)
#endif


#if defined(_DEBUG)
// Logs information string to Output Window.
// @param format - string literal with optional formatting.
#define LOG_INFO(format, ...) LOG(LOG_LEVEL_INFO, format, __VA_ARGS__)
#else
#define LOG_INFO(format, ...)
#endif


#if defined(_DEBUG)
// Logs warning string to Output Window.
// @param format - string literal with optional formatting.
#define LOG_WARNING(format, ...) LOG(LOG_LEVEL_WARNING, format, __VA_ARGS__)
#else
#define LOG_WARNING(format, ...)
#endif


#if defined(_DEBUG)
// Logs error string to Output Window.
// @param format - string literal with optional formatting.
#define LOG_ERROR(format, ...) LOG(LOG_LEVEL_ERROR, format, __VA_ARGS__)
#else
#define LOG_ERROR(format, ...)
#endif

// Debug assert test.
#if defined(_DEBUG)
#define ASSERT(x) assert(x)
#else
#define ASSERT(x)
#endif

#if defined(_DEBUG)
#define CHECK_RESULT(x, str)					\
	do {										\
		HRESULT result = (x);					\
		if ( FAILED(result) ) {					\
			std::cout << str << __FILE__ <<     \
				":" << __LINE__ << std::endl;	\
			assert(result == S_OK);				\
		}										\
	}											\
	while(0)
#else
#define CHECK_D3D_RESULT(x) x;
#endif

#if defined(_DEBUG)
#define CHECK_D3D_RESULT(x) CHECK_RESULT(x, "Direct3D error at ");
#else
#define CHECK_D3D_RESULT(x) x;
#endif

#if defined(_DEBUG)
#define CHECK_WIN_RESULT(x) CHECK_RESULT(x, "Windows error at ");
#else
#define CHECK_WIN_RESULT(x) x;
#endif


#if defined(_DEBUG)
	// Assigns a default name to a single D3D12 object to aid in identification
	// of the object during graphics debugging.
	// @param x - pointer to a D3D12 object type.
#define SET_D3D12_DEBUG_NAME(x) \
	x->SetName(L#x);
#else
#define SET_D3D12_DEBUG_NAME(x)
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

// Force runtime to break with optional message.
#define ForceBreak(message) \
	do { \
		OutputDebugStringA("Error: " message "\n"); \
		__debugbreak(); \
	} while(0)


