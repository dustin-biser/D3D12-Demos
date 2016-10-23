//
// ImageDecoder.hpp
//

#pragma once

#include <vector>

#include "NumericTypes.hpp"


struct ImageData {
	ImageData();

	~ImageData();

	int width;
	int height;
	size_t numBytes;
	byte * data;
};


namespace ImageDecoder {

	void decodeImage (
		const std::wstring & path,
		int rowAlignment,
		ImageData * imageData
	);

};
