//
// ImageDecoder.cpp
//

#include "pch.h"

#include "ImageDecoder.hpp"

#include <wrl.h>
#include <wincodec.h>

using Microsoft::WRL::ComPtr;



//---------------------------------------------------------------------------------------
ImageData::ImageData()
	: data(nullptr),
	  numBytes(0),
	  width(0),
	  height(0)
{

}

//---------------------------------------------------------------------------------------
ImageData::~ImageData()
{
	delete[] data;
}



//---------------------------------------------------------------------------------------
static void decodeImageStream (
	IWICImagingFactory * factory,
	IWICStream * stream,
	const int rowAlignment,
	ImageData * imageData
) {
	// Create decoder, which will decode stream into a bitmap format.
	ComPtr<IWICBitmapDecoder> decoder;
	CHECK_WIN_RESULT (
		factory->CreateDecoderFromStream (
			stream, nullptr,
			WICDecodeMetadataCacheOnDemand,
			&decoder
		)
	)

	ComPtr<IWICBitmapFrameDecode> frame;
	// Decode the first frame.
	CHECK_WIN_RESULT (
		decoder->GetFrame(0, &frame)
	);

	ComPtr<IWICFormatConverter> converter;
	CHECK_WIN_RESULT (
		factory->CreateFormatConverter(&converter)
	);
	CHECK_WIN_RESULT (
		converter->Initialize (
			frame.Get(), GUID_WICPixelFormat32bppRGBA,
			WICBitmapDitherTypeNone, nullptr, 0.f,
			WICBitmapPaletteTypeMedianCut
		)
	);

	UINT width, height;
	CHECK_WIN_RESULT (
		converter->GetSize(&width, &height)
	);

	const uint bytesPerPixel = 4;
	const uint numBytes = width * height * bytesPerPixel;

	imageData->width = static_cast<int>(width);
	imageData->height = static_cast<int>(height);
	imageData->numBytes = numBytes;

	// Allocate storage for image data.
	imageData->data = new byte[numBytes];

	const uint stride = width * bytesPerPixel;

	CHECK_WIN_RESULT (
		converter->CopyPixels (
			nullptr,
			stride,
			static_cast<uint>(imageData->numBytes),
			imageData->data
		)
	);
}

////---------------------------------------------------------------------------------------
//static std::string getImageFileFormat (
//	const char * path
//) {
//	// Copy path
//	std::string fileTypeString(path);
//
//	// Get index of last dot in path, right before file type postfix.
//	size_t index = fileTypeString.find_last_of(".");
//	if (index == std::string::npos) {
//		ForceBreak("Error decoding image. Image path missing file type.");
//	}
//
//	// Reduce path string down to only the file type.
//	fileTypeString = fileTypeString.substr(index + 1);
//
//	// Set all characters of file type to lowercase.
//	for (auto & c : fileTypeString) {
//		c = tolower(c);
//	}
//
//	return fileTypeString;
//}

//---------------------------------------------------------------------------------------
void ImageDecoder::decodeImage(
	const std::string & path,
	int rowAlignment,
	ImageData * imageData
) {
	assert(imageData);

	ComPtr<IWICImagingFactory> factory;

	// Create WICImagingFactory instance.
	CHECK_WIN_RESULT (
		CoCreateInstance (
			CLSID_WICImagingFactory,
			NULL,
			CLSCTX_INPROC_SERVER,
			IID_PPV_ARGS (&factory)
		)
	);

	// Initialize image stream to read file data.
	ComPtr<IWICStream> stream;
	CHECK_WIN_RESULT (
		factory->CreateStream(&stream)
	);

	std::wstring pathWString = toWString (path);
	CHECK_WIN_RESULT (
		stream->InitializeFromFilename(pathWString.c_str(), GENERIC_READ)
	);

	decodeImageStream(factory.Get(), stream.Get(), rowAlignment, imageData);
}
