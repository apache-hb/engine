cbuffer Data : register(b0) {
    float4 offset;
    float4x4 mvp;
};

struct PSInput {
    float4 pos : SV_POSITION;
    float4 colour : COLOUR;
};

float4 perspective(float3 position) {
    return mul(float4(position, 1.0f), mvp);
}

PSInput vsMain(float3 pos : POSITION, float4 colour : COLOUR) {
    PSInput result;
    result.pos = perspective(pos);
    result.colour = colour;
    return result;
}

float4 psMain(PSInput input) : SV_TARGET {
    return input.colour;
}
