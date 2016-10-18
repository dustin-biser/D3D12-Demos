#include "PSInput.hlsli"
#include "../../ConstantBufferDefines.hpp"

ConstantBuffer<DirectionalLight> light : register(b0, space1);

float4 PSMain (PSInput input) : SV_TARGET
{
    return float4(0.8, 0.4, 0.1, 0.0f);
}
