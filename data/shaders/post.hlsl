struct Data {
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
};

Texture2D gTexture : register(t0);
SamplerState gSampler : register(s0);

Data vsMain(float3 position : POSITION, float2 uv : TEXCOORD0) {
    Data result;
    result.pos = float4(position, 1.0);
    result.uv = uv;
    return result;
}

float4 psMain(Data input) : SV_TARGET {
    return gTexture.Sample(gSampler, input.uv);
}
