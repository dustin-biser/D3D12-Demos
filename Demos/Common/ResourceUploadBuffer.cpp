#include "pch.h"
using Microsoft::WRL::ComPtr;

#include <exception>

#include "ResourceUploadBuffer.hpp"

//-- Minimum byte alignments for common D3D12 Resource types:
// Note that index data alignment is equal to sizeof(index), so 2 for 16bit indices,
// or 4 for 32bit indices, etc..
static const uint VertexData_Alignment = 4;
static const uint ConstantBufferData_Alignment = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;
static const uint TextureData_Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
static const uint BufferData_Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
static const uint MSAAResourceData_Alignment = D3D12_DEFAULT_MSAA_RESOURCE_PLACEMENT_ALIGNMENT;


// Function Declaration
template <typename T>
static T align(T value, T alignment);


class ResourceUploadBufferImpl {
private:
	friend class ResourceUploadBuffer;

	// The Resource backing the buffer storage.
	ComPtr<ID3D12Resource> resourceBuffer;
	
	// Starting position of resource buffer.
	byte * dataBegin = nullptr;  

    // Current position within resource buffer for sub-allocations.
	byte * dataCur = nullptr;

    // Ending position of resource buffer.
	byte * dataEnd = nullptr;

	// GPU virtual address to beginning of upload buffer
	D3D12_GPU_VIRTUAL_ADDRESS uploadBufferGPUVirtualAddress;


	void alignDataPointerForAllocation(size_t alignSize);

	void uploadData (
		_In_ const void * data,
		_In_ size_t dataBytes,
		_In_ size_t alignment,
		_Out_ size_t & dataGPUVirtualAddress,
		_Out_opt_ void ** mappedDataPtr = nullptr
	);
};

//---------------------------------------------------------------------------------------
void ResourceUploadBufferImpl::alignDataPointerForAllocation (
	size_t alignSize
) {
	size_t alignedValue = align(reinterpret_cast<size_t>(this->dataCur), alignSize);
	this->dataCur = reinterpret_cast<byte *>(alignedValue);
}

//---------------------------------------------------------------------------------------
ResourceUploadBuffer::ResourceUploadBuffer (
	ID3D12Device * device,
	uint64 sizeInBytes
) {
	impl = new ResourceUploadBufferImpl();

	CHECK_D3D_RESULT (
		device->CreateCommittedResource (
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(sizeInBytes),
			D3D12_RESOURCE_STATE_GENERIC_READ, 
			nullptr,
			IID_PPV_ARGS(&impl->resourceBuffer)
		)
	);
	D3D12_SET_NAME(impl->resourceBuffer, L"ResourceUploadBuffer");
	impl->uploadBufferGPUVirtualAddress = impl->resourceBuffer->GetGPUVirtualAddress();

	//-- Map the full buffer range, so CPU can write to it.
	void * bufferStart;
	CD3DX12_RANGE readRange(0,0); // No CPU reads will be done from the resource.
	impl->resourceBuffer->Map(0, /*&readRange*/ nullptr, &bufferStart);

	impl->dataCur = impl->dataBegin = reinterpret_cast<byte *>(bufferStart);
	impl->dataEnd = impl->dataBegin + sizeInBytes;
}


//---------------------------------------------------------------------------------------
ResourceUploadBuffer::~ResourceUploadBuffer()
{
	impl->resourceBuffer->Unmap(0, nullptr);
	delete impl;
}

//---------------------------------------------------------------------------------------
void ResourceUploadBufferImpl::uploadData (
	_In_ const void * data,
	_In_ size_t dataBytes,
	_In_ size_t alignment,
	_Out_ size_t & dataGPUVirtualAddress,
	_Out_opt_ void ** mappedDataPtr
) {
	alignDataPointerForAllocation(alignment);

	if ((dataCur + dataBytes) > dataEnd) {
		ForceBreak("Insufficient memory.  Unable to upload data.");
	}
	if (mappedDataPtr) {
		*mappedDataPtr = dataCur;
	}

	// Compute byte offset from start of upload buffer to beginning of copied data.
	size_t byteOffsetToData = size_t(dataCur - dataBegin);

	// Compute GPU virtual address to start of copied data.
	dataGPUVirtualAddress = uploadBufferGPUVirtualAddress + byteOffsetToData;

	memcpy(dataCur, data, dataBytes);
	dataCur += dataBytes;
}

//---------------------------------------------------------------------------------------
void ResourceUploadBuffer::uploadVertexData (
	_In_ const void * data,
	_In_ size_t dataBytes,
	_In_ size_t sizeOfVertex,
	_Out_ D3D12_VERTEX_BUFFER_VIEW & vertexBufferView,
	_Out_opt_ void ** mappedDataPtr
) {
	size_t dataGPUVirtualAddress(0);

	impl->uploadData (
		data,
		dataBytes,
		VertexData_Alignment,
		dataGPUVirtualAddress,
		mappedDataPtr
	);

	vertexBufferView.SizeInBytes = static_cast<UINT>(dataBytes);
	vertexBufferView.StrideInBytes = static_cast<UINT>(sizeOfVertex);
	vertexBufferView.BufferLocation = dataGPUVirtualAddress;
}

//---------------------------------------------------------------------------------------
void ResourceUploadBuffer::uploadIndexData (
	_In_ const void * data,
	_In_ size_t dataBytes,
	_In_ size_t sizeOfIndex,
	_Out_ D3D12_INDEX_BUFFER_VIEW & indexBufferView,
	_Out_opt_ void ** mappedDataPtr
) {
	size_t dataGPUVirtualAddress(0);
	size_t dataAlignment = sizeOfIndex;

	DXGI_FORMAT dxgiIndexFormat;
	switch (sizeOfIndex) {
	case 1:
		dxgiIndexFormat = DXGI_FORMAT_R8_UINT;
		break;
	case 2:
		dxgiIndexFormat = DXGI_FORMAT_R16_UINT;
		break;
	case 4:
		dxgiIndexFormat = DXGI_FORMAT_R32_UINT;
		break;
	default:
		ForceBreak("Invalid Index Format.");
		break;
	}

	impl->uploadData (
		data,
		dataBytes,
		dataAlignment,
		dataGPUVirtualAddress,
		mappedDataPtr
	);

	indexBufferView.SizeInBytes = static_cast<UINT>(dataBytes);
	indexBufferView.BufferLocation = dataGPUVirtualAddress;
	indexBufferView.Format = dxgiIndexFormat;
}

//---------------------------------------------------------------------------------------
void ResourceUploadBuffer::uploadConstantBufferData (
	_In_ const void * data,
	_In_ size_t dataBytes,
	_Out_ D3D12_CONSTANT_BUFFER_VIEW_DESC & cbvDesc,
	_Out_opt_ void ** mappedDataPtr
) {
	size_t dataGPUVirtualAddress(0);

	impl->uploadData(
		data,
		dataBytes,
		ConstantBufferData_Alignment,
		dataGPUVirtualAddress,
		mappedDataPtr
	);

	cbvDesc.BufferLocation = dataGPUVirtualAddress;
	cbvDesc.SizeInBytes = align(static_cast<uint>(dataBytes), ConstantBufferData_Alignment);
}

//---------------------------------------------------------------------------------------
D3D12_GPU_VIRTUAL_ADDRESS ResourceUploadBuffer::getGPUVirtualAddress() const
{
	return impl->resourceBuffer->GetGPUVirtualAddress();
}

//---------------------------------------------------------------------------------------
// Align value to the next multiple of alignment.
template <typename T>
static T align(T value, T alignment)
{
	if ((0 == alignment) || (alignment & (alignment - 1))) {
		ForceBreak("Error. Non-power of 2 alignment requested.");
	}

	return ((value + (alignment - 1)) & ~(alignment - 1));
}
