cbuffer GlobalBuffer : register(b0) {
    float4x4 mvp;
};

cbuffer LocalBuffer : register(b1) {
    uint texture;
};

struct PSInput {
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
};

Texture2D gTextures[] : register(t0);
SamplerState gSampler : register(s0);

float4 perspective(float3 pos) {
    return mul(float4(pos, 1.f), mvp);
}

PSInput vsMain(float3 pos : POSITION, float2 uv : TEXCOORD) {
    PSInput result;
    result.pos = perspective(pos);
    result.uv = uv;
    return result;
}

float4 psMain(PSInput input) : SV_TARGET {
    return gTextures[texture].Sample(gSampler, input.uv);
}
