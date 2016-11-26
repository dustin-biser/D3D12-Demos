#include "PSInput.hlsli"
#include "../../ConstantBufferDefines.hpp"


Texture2D<float4> imageTexture : register(t0);
SamplerState texureSampler     : register(s0);

float4 PSMain (PSInput psInput) : SV_TARGET 
{
    return imageTexture.Sample(texureSampler, psInput.texCoord);
}

