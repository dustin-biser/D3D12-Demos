#pragma once

#include <intrin.h>
#include <cwchar>
#include <iostream>
#include <debugapi.h>

#define LOG_BUFFER_LENGTH 256
#define LOG_LEVEL_INFO "Log Info: "
#define LOG_LEVEL_WARNING "Log Warning: "
#define LOG_LEVEL_ERROR "Log Error: "

#if defined(_DEBUG)
#define LOG(levelWString, format, ...) \
	do { \
		char buffer[LOG_BUFFER_LENGTH]; \
		int charsWritten = sprintf(buffer, levelWString); \
		charsWritten += sprintf(buffer + charsWritten, format, __VA_ARGS__); \
		sprintf(buffer + charsWritten, "\n"); \
		OutputDebugString(buffer); \
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


#if defined(_DEBUG)
#define	Assert( x )  do { if ( !(x) ) { LOG_ERROR("Assert Failed. Expression '" #x "' is false or 0."); __debugbreak(); } } while (false)
#else
#define	Assert( x )  (void)x;
#endif // #if defined(_DEBUG)


#if defined(_DEBUG)
#define CHECK_RESULT(x, str)					\
	do {										\
		HRESULT result = (x);					\
		if ( FAILED(result) ) {					\
			std::cout << str << __FILE__ <<     \
				":" << __LINE__ << std::endl;	\
			Assert(result == S_OK);				\
		}										\
	}											\
	while (false)
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
			wchar_t buffer[256]; \
			swprintf(buffer, 256, L"%ls[%u]", L#x, i); \
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


#define RELEASE_NULLIFY( pComObject )  do { \
	Assert( pComObject ); \
	pComObject->Release(); \
	pComObject = nullptr; \
} while (false)


std::wstring toWString (
	const std::string & str
);

void GetWorkingDir (
	_Out_writes_(pathSize) char * path,
	UINT pathSize
);


void GetSolutionDir (
	_Out_writes_(pathSize) char * path,
	UINT pathSize
);


/// Query a D3D12Device to determine memory usage information.
void QueryVideoMemoryInfo (
	_In_ ID3D12Device * device,
	_In_ DXGI_MEMORY_SEGMENT_GROUP memoryGroup,
	_Out_ DXGI_QUERY_VIDEO_MEMORY_INFO & videoMemoryInfo
);

// Force runtime to break with optional message.
#define ForceBreak(message, ...) \
	do { \
		LOG_ERROR(message, __VA_ARGS__); \
		__debugbreak(); \
	} while(0)


