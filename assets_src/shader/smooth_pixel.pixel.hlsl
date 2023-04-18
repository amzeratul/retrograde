#include "halley/sprite_attribute.hlsl"

Texture2D tex0 : register(t0);
SamplerState sampler0 : register(s0);

SamplerState linearSampler : register(s1)
{
    Filter = MIN_MAG_LINEAR_MIP_POINT;
    AddressU = Clamp;
    AddressV = Clamp;
};

float4 smoothPixel(Texture2D tex, float2 uv, float sharpness)
{
    int2 size;
    tex.GetDimensions(size.x, size.y);
    float2 pxCoord = size * uv;
    float2 pxCoordMid = floor(pxCoord) + float2(0.5, 0.5);
    float2 pxCoordFract = pxCoord - pxCoordMid;

    float pxSize = length(ddx(pxCoord));
    float zoom = 1.0 / pxSize * sharpness;

    if (zoom < 1) {
        return tex.Sample(linearSampler, uv);
    } else {    
        float2 fractAdjust = max(abs(pxCoordFract) * zoom - ((zoom - 1.0) / 2.0).rr, float2(0, 0)) * sign(pxCoordFract);
        float2 sampleCoord = (pxCoordMid + fractAdjust) / size;

        return tex.Sample(linearSampler, sampleCoord);
    }
}

float4 main(VOut input) : SV_TARGET {
    float4 col = smoothPixel(tex0, input.texCoord0.xy, 1.0);
    return col * input.colour + input.colourAdd * col.a;
}
