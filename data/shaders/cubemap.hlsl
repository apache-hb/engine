struct Vertex {
    float4 position : SV_POSITION;
    float3 uv : TEXCOORD;
};

cbuffer SceneBuffer : register(b0) {
    float4x4 mvp;
};

TextureCube gCubeMap : register(t0);
SamplerState gSampler : register(s0);

float4 perspective(float3 pos) {
    return mul(float4(pos, 1.0), mvp);
}

Vertex vsMain(float3 pos : POSITION) {
    Vertex result;
    result.position = perspective(pos);
    result.uv = pos;
    return result;
}

float4 psMain(Vertex input) : SV_TARGET {
    return gCubeMap.Sample(gSampler, input.uv);
}
