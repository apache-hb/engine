cbuffer SceneBuffer : register(b0) {
    float4x4 camera; // camera matrix, column major
    float3 light; // light position
};

cbuffer ObjectBuffer : register(b1) {
    float4x4 transform; // node transform
};

cbuffer PrimitiveBuffer : register(b2) {
    uint texture; // texture index
};

struct PSInput {
    float4 pos : SV_POSITION;
    float4 normal : NORMAL;
    float2 uv : TEXCOORD;
};

Texture2D gTextures[] : register(t0);
SamplerState gSampler : register(s0);

float4 perspective(float3 pos) {
    return mul(mul(float4(pos, 1.f), transform), camera);
}

PSInput vsMain(float3 pos : POSITION, float3 normal : NORMAL, float2 uv : TEXCOORD) {
    PSInput result;
    result.pos = perspective(pos);
    result.normal = float4(normal, 0.f);
    result.uv = uv;
    return result;
}

float4 psMain(PSInput input) : SV_TARGET {
    // diffuse lighting
    float3 lightDir = normalize(light - input.pos.xyz);
    float diffuse = max(0.f, dot(input.normal.xyz, lightDir));

    // texture
    float4 color = gTextures[texture].Sample(gSampler, input.uv);

    return float4(color.rgb * diffuse, color.a);
}
