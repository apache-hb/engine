struct Vertex {
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
};

Texture2D gTexture : register(t0);
SamplerState gSampler : register(s0);

Vertex vsMain(float3 pos : POSITION, float2 uv : TEXCOORD) {
    Vertex result;
    result.position = float4(pos, 1.0);
    result.uv = uv;
    return result;
}

float4 psMain(Vertex input) : SV_TARGET {
    return gTexture.Sample(gSampler, input.uv);
}
