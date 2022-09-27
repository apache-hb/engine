cbuffer GlobalBuffer : register(b0) {
    float3 offset;
};

struct PSInput {
    float4 pos : SV_POSITION;
    float4 colour : COLOUR;
};

PSInput vsMain(float3 pos : POSITION, float4 colour : COLOUR) {
    PSInput result;
    result.pos = float4(pos + offset, 1.f);
    result.colour = colour;
    return result;
}

float4 psMain(PSInput input) : SV_TARGET {
    return input.colour;
}
