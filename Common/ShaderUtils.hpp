//
// ShaderUtils.hpp
//
#pragma once

#include <d3d12.h>

class ShaderSource {
public:
	~ShaderSource ();

	D3D12_SHADER_BYTECODE byteCode;
};


/// Loads compiled shader object (cso) bytecode file into shaderSource.
void LoadCompiledShaderFromFile (
	const char * csoFile,
	ShaderSource & shaderSource
);
