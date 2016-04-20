/*
 * VertexShader.hlsl
 */

#include "PSInput.hlsli"

// TODO - Add model, view, and perspective matrices.

PSInput VSMain(float4 position : POSITION, float4 normal : NORMAL)
{
	PSInput result;

	result.position = position;
	result.color = normal; 

	return result;
}

