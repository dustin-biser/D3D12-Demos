#ifndef _PSINPUT_HLSLI_
#define _PSINPUT_HLSLI_

struct PSInput
{
	float4 position_clipSpace : SV_POSITION;
    float4 position_eyeSpace : POSITION;
    float3 normal_eyeSpace : NORMAL;
};

#endif // _PSINPUT_HLSLI_