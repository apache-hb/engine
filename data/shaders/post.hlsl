struct Vertex {
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
};

Texture2D gTexture : register(t0);
SamplerState gSampler : register(s0);

Vertex vsMain(float3 position : POSITION, float2 uv : TEXCOORD0) {
    Vertex result;
    result.pos = float4(position, 1.0);
    result.uv = uv;
    return result;
}

float4 psMain(Vertex input) : SV_TARGET {
    return gTexture.Sample(gSampler, input.uv);
}
