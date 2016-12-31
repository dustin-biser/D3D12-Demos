#ifndef _HLSL_DIRECTXMATH_CONVERSION_HPP_
#define _HLSL_DIRECTXMATH_CONVERSION_HPP_

#ifdef __cplusplus
	#define PACKOFFSET(x)

	#include <DirectXMath.h>
	typedef DirectX::XMFLOAT4X4 mat4;
	typedef DirectX::XMFLOAT3X3 mat3;

	typedef DirectX::XMFLOAT4 float4;
	typedef DirectX::XMFLOAT3 float3;

#else // HLSL
	#define PACKOFFSET(x) packoffset(x)

	typedef float4x4 mat4;
	typedef float3x3 mat3;

#endif


#endif//_HLSL_DIRECTXMATH_CONVERSION_HPP_