//
// ImageIO.hpp
//

#pragma once

#include <vector>

#include "Common/Types.hpp"

namespace ImageIO
{
	std::vector<byte> LoadImageFromFile (
		const char * path,
		const int rowAlignment,
		int * width,
		int * height
	);

	std::vector<byte> LoadImageFromMemory (
		const void * imageData,
		const std::size_t size,
		const int rowAlignment,
		int * width,
		int * height
	);

}