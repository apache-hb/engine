SamplerState gSampler : register(s0);
Texture2D gTexture : register(t0);

struct PSInput {
    float4 pos : SV_POSITION;
    float4 colour : COLOUR;
};

PSInput vsMain(float3 pos : POSITION, float4 colour : COLOUR) {
    PSInput result;
    result.pos = float4(pos, 1.f);
    result.colour = colour;
    return result;
}

float4 psMain(PSInput input) : SV_TARGET {
    return input.colour;
}
