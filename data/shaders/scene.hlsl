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

Texture2D gTextures[] : register(t0);
SamplerState gSampler : register(s0);

float4 perspective(float4 position) {
    return mul(position, mvp);
}

Vertex vsMain(float3 pos : POSITION, float2 uv : TEXCOORD) {
    Vertex vertex;
    vertex.position = perspective(float4(pos, 1.0));
    vertex.uv = uv;
    return vertex;
}

float4 psMain(Vertex vertex) : SV_TARGET {
    return gTextures[texture].Sample(gSampler, vertex.uv);
}
