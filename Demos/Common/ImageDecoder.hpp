//
// ImageDecoder.hpp
//

#pragma once

#include <vector>

#include "Common/BasicTypes.hpp"


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
		const std::string & path,
		int rowAlignment,
		ImageData * imageData
	);

};
