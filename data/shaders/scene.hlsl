struct Vertex {
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
};

cbuffer SceneBuffer : register(b0) {
    float4x4 mvp;
};

cbuffer MaterialBuffer : register(b1) {
    uint texture;
}

cbuffer NodeBuffer : register(b2) {
    float4x4 transform;
};

float4 perspective(float3 pos) {
    return mul(float4(pos, 1.0), mvp);
}

Texture2D gTextures[] : register(t0);
SamplerState gSampler : register(s0);

Vertex vsMain(float3 pos : POSITION, float2 uv : TEXCOORD) {
    Vertex vertex;
    vertex.position = perspective(pos);
    vertex.uv = uv;
    return vertex;
}

float4 psMain(Vertex vertex) : SV_TARGET {
    return gTextures[texture].Sample(gSampler, vertex.uv);
}
