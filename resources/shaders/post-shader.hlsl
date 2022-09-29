struct PSInput {
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
};

Texture2D gTexture : register(t0);
SamplerState gSampler : register(s0);

PSInput vsMain(float3 pos : POSITION, float2 uv : TEXCOORD) {
    PSInput result;
    result.pos = float4(pos, 1.f);
    result.uv = uv;
    return result;
}

float4 psMain(PSInput input) : SV_TARGET {
    return gTexture.Sample(gSampler, input.uv);
}
