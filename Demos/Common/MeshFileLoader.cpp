//
// ObjFileLoader.cpp
//
#include "pch.h"

#include "Common/MeshFileLoader.hpp"

#include <fstream>

void MeshFileLoader::loadObjAsset (
	const char * objFilePath,
	Mesh & mesh
) {
	std::ifstream fileStream (objFilePath, std::ios::in);
	if (!fileStream) {
		ForceBreak ("Unable to open obj asset file: %s", objFilePath);
	}

	char buffer[256];

	while (!fileStream.eof ()) {
		fileStream.getline (buffer, 256);
	}
}
