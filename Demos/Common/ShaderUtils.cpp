#include "pch.h"

#include "Common\ShaderUtils.hpp"

#include <fstream>

//---------------------------------------------------------------------------------------
ShaderSource::~ShaderSource ()
{
	delete byteCode.pShaderBytecode;
}


//---------------------------------------------------------------------------------------
void LoadCompiledShaderFromFile (
	const char * csoFile,
	ShaderSource & shaderSource
) {
	// Open file, and advance read position to end of file.
	std::ifstream file (csoFile, std::ios::binary | std::ios::ate);

	// Get current read position within input stream, which is the size in bytes of file.
	std::streamsize size = file.tellg ();

	// Reposition read pointer to beginning of file.
	file.seekg (0, std::ios::beg);

	char * bytes = new char[size];

	// Read file into bytes array.
	if (!(file.read (bytes, size))) {
		ForceBreak ("Unable to read all bytes from shader .cso file.");
	}

	shaderSource.byteCode.BytecodeLength = size;
	shaderSource.byteCode.pShaderBytecode = bytes;
}
