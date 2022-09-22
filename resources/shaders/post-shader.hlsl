cbuffer Data : register(b0) {
    float4 offset;
    float4x4 mvp;
};

struct PSInput {
    float4 pos : SV_POSITION;
    float4 colour : COLOUR;
};

PSInput vsMain(float4 pos : POSITION, float4 colour : COLOUR) {
    PSInput result;
    result.pos = pos + offset;
    result.colour = colour;
    return result;
}

float4 psMain(PSInput input) : SV_TARGET {
    return input.colour;
}