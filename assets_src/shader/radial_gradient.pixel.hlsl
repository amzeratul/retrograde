#include "halley/sprite_attribute.hlsl"

float randomNoise(float2 seed)
{
    return frac(sin(dot(sin(seed), float2(12.9898, 78.233))) * 107137.0191);
}

float4 main(VOut input) : SV_TARGET {
    float ar = ddx(input.texCoord0.x) / ddy(input.texCoord0.y);

    float2 coord = (input.texCoord0.xy * 2.0 - float2(1, 1)) / float2(ar, 1);
    float dist = length(coord) / sqrt(1.0 / ar + 1.0);
    float4 edgeCol = float4(input.colour.rgb, 1.0);
    float4 midCol = float4(0.14, 0.11, 0.11, 1.0); // Hack
    float n = randomNoise(input.position) / 255.0;
    return (lerp(midCol, edgeCol, dist) + float4(n, n, n, n)) * input.colour.aaaa;
}
