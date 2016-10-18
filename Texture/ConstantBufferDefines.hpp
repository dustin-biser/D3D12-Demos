#ifndef _CONSTANT_BUFFER_DEFINES_HPP_ 
#define _CONSTANT_BUFFER_DEFINES_HPP_ 

#include "HLSL_DirectXMath_Conversion.hpp"

struct SceneConstants
{
    mat4 modelViewMatrix;
    mat4 MVPMatrix;
    mat4 normalMatrix;
};

struct DirectionalLight
{
	float4 direction;
	float4 color;
};

#endif // _CONSTANT_BUFFER_DEFINES_HPP_ 