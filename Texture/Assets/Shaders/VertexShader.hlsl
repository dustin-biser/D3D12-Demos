#include "PSInput.hlsli"
#include "../../ConstantBufferDefines.hpp"

ConstantBuffer<SceneConstants> sceneConstants : register(b0, space0);

PSInput VSMain (
    float3 position : POSITION,
    float3 normal : NORMAL
) {
	PSInput psInput;

    // Transform vertex position to Eye Space and Clip Space.
    float4 vertexPosition = float4(position, 1.0f);
    psInput.position_eyeSpace = mul(vertexPosition, sceneConstants.modelViewMatrix);
    psInput.position_clipSpace = mul(vertexPosition, sceneConstants.MVPMatrix);

    // Transform normal to Eye Space.
    psInput.normal_eyeSpace = 
        mul(float4(normal, 0.0f), sceneConstants.normalMatrix).xyz;

	return psInput;
}

