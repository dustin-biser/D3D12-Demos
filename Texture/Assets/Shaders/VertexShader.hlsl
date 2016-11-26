#include "PSInput.hlsli"
#include "../../ConstantBufferDefines.hpp"

ConstantBuffer<SceneConstants> sceneConstants : register(b0, space0);

PSInput VSMain (
    float3 position : POSITION,
    float3 normal   : NORMAL,
    float2 texCoord : TEXCOORD
) {
	PSInput psInput;
    psInput.texCoord = texCoord;

    psInput.position_clipSpace = float4(position, 1.0);
    psInput.position_clipSpace.x *= sceneConstants.inv_aspectRatio;

	return psInput;
}

