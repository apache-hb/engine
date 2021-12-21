Texture2D tex : register(t0);
SamplerState state : register(s0);

struct PSInput {
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
};

PSInput vsMain(float4 position : POSITION, float2 uv : TEXCOORD) {
    PSInput result;
    result.position = position;
    result.uv = uv;
    return result;
}

float4 psMain(PSInput input) : SV_TARGET {
    return tex.Sample(state, input.uv);
}
