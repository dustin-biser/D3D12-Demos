#include "PSInput.hlsli"
#include "ConstantBufferDefines.hpp"

ConstantBuffer<PointLight> light : register(b0, space1);

float4 PSMain(PSInput input) : SV_TARGET
{
    const float3 ambient = { 0.02f, 0.02f, 0.02f };
    const float3 Kd = { 1.0f, 0.3f, 0.3f };

    float3 lightDir = light.position_eyeSpace.xyz - input.position_eyeSpace.xyz;
    lightDir = normalize(lightDir);

    float3 normal = normalize(input.normal_eyeSpace);

    float3 diffuse = light.color.xyz * Kd * max(dot(normal, lightDir), 0.0f);
    diffuse = saturate(diffuse);

    return float4(ambient + diffuse, 0.0f);
}
