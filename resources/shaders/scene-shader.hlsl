/// only needs to be bound for the vertex shader stage
cbuffer ConstBuffer : register(b0) {
    float4x4 model;
    float4x4 view;
    float4x4 projection;
};

/// only need to be bound for the pixel shader stage
Texture2D textures[] : register(t0);
SamplerState texSampler : register(s0);

struct PSInput {
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
    uint tex : TEXINDEX;
};

float4 perspective(float3 position) {
    float4 pos = float4(position, 1.0f);
    pos = mul(pos, model);
    pos = mul(pos, view);
    pos = mul(pos, projection);
    return pos;
}

PSInput vsMain(float3 position : POSITION, float3 normal : NORMAL, float2 uv : TEXCOORD, uint tex : TEXINDEX) {    
    PSInput result;

    result.position = perspective(position);
    result.normal = normal;
    result.uv = uv;
    result.tex = tex;

    return result;
}

float4 psMain(PSInput input) : SV_TARGET {
    return textures[input.tex].Sample(texSampler, input.uv);
}
