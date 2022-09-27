cbuffer GlobalBuffer : register(b0) {
    float4x4 mvp;
};

struct PSInput {
    float4 pos : SV_POSITION;
    float4 colour : COLOUR;
};

float4 perspective(float3 pos) {
    return mul(float4(pos, 1.f), mvp);
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
