#pragma once

#include <d3d12.h>
#include "NumericTypes.hpp"

// Forward Declaration
class ResourceUploadBufferImpl;

/**
* Class for managing an upload heap buffer for uploading resources that are 
* typically write-once, read-once GPU data.
*/
class ResourceUploadBuffer {
public:

	ResourceUploadBuffer (
		ID3D12Device * device,
		uint64 sizeInBytes
	);

	~ResourceUploadBuffer();

	void uploadVertexData (
		_In_ const void * data,
		_In_ size_t dataBytes,
		_In_ size_t sizeOfVertex,
		_Out_ D3D12_VERTEX_BUFFER_VIEW & vertexBufferView,
		_Out_ void ** mappedDataPtr = nullptr
	);

	void uploadIndexData (
		_In_ const void * data,
		_In_ size_t dataBytes,
		_In_ size_t sizeOfIndex,
		_Out_ D3D12_INDEX_BUFFER_VIEW & indexBufferView,
		_Out_ void ** mappedDataPtr = nullptr
	);

	void uploadConstantBufferData (
		_In_ const void * data,
		_In_ size_t dataBytes,
		_Out_ D3D12_CONSTANT_BUFFER_VIEW_DESC & cbvDesc,
		_Out_ void ** mappedDataPtr = nullptr
	);

	D3D12_GPU_VIRTUAL_ADDRESS getGPUVirtualAddress() const;


private:
	ResourceUploadBufferImpl * impl;
};

