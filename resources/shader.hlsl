/// only needs to be bound for the vertex shader stage
cbuffer ConstBuffer : register(b0) {
    float4x4 model;
    float4x4 view;
    float4x4 projection;
};

/// only need to be bound for the pixel shader stage
Texture2D texResource : register(t0);
SamplerState texSampler : register(s0);

struct PSInput {
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
};

float4 perspective(float3 position) {
    float4 pos = float4(position, 1.0f);
    pos = mul(pos, model);
    pos = mul(pos, view);
    pos = mul(pos, projection);
    return pos;
}

PSInput VSMain(float3 position : POSITION, float3 normal : NORMAL, float2 uv : TEXCOORD) {    
    PSInput result;

    result.position = perspective(position);
    result.normal = normal;
    result.uv = uv;

    return result;
}

float4 PSMain(PSInput input) : SV_TARGET {
    return texResource.Sample(texSampler, input.uv);
}
