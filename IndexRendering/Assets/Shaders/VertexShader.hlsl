/*
 * VertexShader.hlsl
 */

#include "PSInput.hlsli"

PSInput VSMain(float4 position : POSITION, float4 color : COLOR)
{
	PSInput result;

	result.position = position;
	result.color = color;

	return result;
}

