//
// ImageIO.cpp
//

#include "pch.h"

#include "ImageIO.hpp"

#include <Windows.h>
#include <wrl.h>
#include <wincodec.h>
// for _com_error
#include <comdef.h>

#include "DemoUtils.hpp"
#define SAFE_WIC(expr) do {const auto r = expr; if (FAILED(r)) {_com_error err (r); OutputDebugString (err.ErrorMessage()); __debugbreak ();} } while (0,0)

using namespace Microsoft::WRL;

//---------------------------------------------------------------------------------------
static std::vector<std::uint8_t> LoadInternal (
	ComPtr<IWICImagingFactory> factory,
	ComPtr<IWICStream> stream,
	const int rowAlignment,
	int * outputWidth,
	int * outputHeight
) {
	ComPtr<IWICBitmapDecoder> decoder;
	factory->CreateDecoderFromStream(stream.Get(), nullptr,
		WICDecodeMetadataCacheOnDemand, &decoder);

	ComPtr<IWICBitmapFrameDecode> frame;
	decoder->GetFrame(0, &frame);

	ComPtr<IWICFormatConverter> converter;
	factory->CreateFormatConverter(&converter);
	converter->Initialize(
		frame.Get(), GUID_WICPixelFormat32bppRGBA,
		WICBitmapDitherTypeNone, nullptr, 0.f,
		WICBitmapPaletteTypeMedianCut);

	UINT width, height;
	converter->GetSize(&width, &height);

	std::vector<std::uint8_t> result(
		RoundToNextMultiple(width, static_cast<UINT> (rowAlignment)) * height * 4);

	SAFE_WIC(converter->CopyPixels(nullptr,
		RoundToNextMultiple(width, static_cast<UINT> (rowAlignment)) * 4,
		static_cast<UINT> (result.size()), result.data()));

	if (outputWidth) {
		*outputWidth = static_cast<int> (width);
	}

	if (outputHeight) {
		*outputHeight = static_cast<int> (height);
	}

	return result;
}


//---------------------------------------------------------------------------------------
std::vector<byte> ImageIO::LoadImageFromFile (
	const char * path,
	const int rowAlignment,
	int * outputWidth,
	int * outputHeight
) {
	ComPtr<IWICImagingFactory> factory;
	HRESULT hr = CoCreateInstance (
		CLSID_WICImagingFactory,
		NULL,
		CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS (&factory)
		);

	if (FAILED(hr)) {
		throw std::runtime_error ("Could not create WIC factory");
	}

	ComPtr<IWICBitmapDecoder> decoder;

	std::vector<wchar_t> pathWchar (::strlen (path) + 1);
	mbstowcs (pathWchar.data (), path, pathWchar.size ());

	ComPtr<IWICStream> stream;
	factory->CreateStream(&stream);
	stream->InitializeFromFilename(pathWchar.data (), GENERIC_READ);

	return LoadInternal(factory, stream, rowAlignment, outputWidth, outputHeight);

}

//---------------------------------------------------------------------------------------
std::vector<byte> ImageIO::LoadImageFromMemory (
	const void * data,
	const std::size_t size,  
	const int rowAlignment,
	int * outputWidth,
	INT * outputHeight
) {
	ComPtr<IWICImagingFactory> factory;
	HRESULT hr = CoCreateInstance(
		CLSID_WICImagingFactory,
		NULL,
		CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(&factory)
		);

	if (FAILED (hr)) {
		throw std::runtime_error ("Could not create WIC factory");
	}

	ComPtr<IWICStream> stream;
	factory->CreateStream(&stream);

	// This is fine here as the memory will live on when the stream is long gone
	stream->InitializeFromMemory(static_cast<BYTE*> (const_cast<void*> (data)), 
		static_cast<DWORD> (size));

	return LoadInternal(factory, stream, rowAlignment, outputWidth, outputHeight);
}
