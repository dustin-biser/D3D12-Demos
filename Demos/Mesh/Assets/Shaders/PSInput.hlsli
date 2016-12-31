#ifndef _PSINPUT_HLSLI_
#define _PSINPUT_HLSLI_


struct PSInput {
    float4 position_clipSpace : SV_POSITION;
    float2 texCoord : TEXCOORD;
};


#endif // _PSINPUT_HLSLI_