cbuffer SceneBuffer : register(b0) {
    float4x4 camera;
};

cbuffer ObjectBuffer : register(b1) {
    float4x4 transform; // node transform
};

cbuffer PrimitiveBuffer : register(b2) {
    uint texture; // texture index
};

struct PSInput {
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
};

Texture2D gTextures[] : register(t0);
SamplerState gSampler : register(s0);

float4 perspective(float3 pos) {
    return mul(mul(float4(pos, 1.f), transform), camera);
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
