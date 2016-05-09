#ifndef _CONSTANT_BUFFER_DEFINES_HPP_ 
#define _CONSTANT_BUFFER_DEFINES_HPP_ 

#include "HLSL_DirectXMath_Conversion.hpp"

struct SceneConstants
{
    mat4 modelViewMatrix;
    mat4 MVPMatrix;
    mat4 normalMatrix;
};

struct PointLight
{
	float4 position_eyeSpace;
	float4 color;
};

#endif // _CONSTANT_BUFFER_DEFINES_HPP_ 