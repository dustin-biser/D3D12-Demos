/*
 * VertexShader.hlsl
 */

#include "PSInput.hlsli"

PSInput VSMain(float4 position : POSITION, float4 normal : NORMAL)
{
	PSInput result;

	result.position = position;
	result.color = normal; // TODO - Change this later.

	return result;
}

