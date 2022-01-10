/// only needs to be bound for the vertex shader stage
/// contains the basic model view projection matricies
cbuffer CameraBuffer : register(b0) {
    float4x4 model;
    float4x4 view;
    float4x4 projection;
};

/// only need to be bound for the pixel shader stage
/// contains the currently bound texture index
cbuffer BindsBuffer : register(b1) {
    uint textureIndex;
    uint samplerIndex;
};

/// we use bindless textures so this is all we need
/// `tex` is used to index into this in the pixel shader
Texture2D textures[] : register(t0);
SamplerState samplers[] : register(s0);

struct PSInput {
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
};

float4 perspective(float3 position) {
    float4 pos = float4(position, 1.0f);
    pos = mul(pos, model);
    pos = mul(pos, view);
    pos = mul(pos, projection);
    return pos;
}

PSInput vsMain(float3 position : POSITION, float2 uv : TEXCOORD) {    
    PSInput result;

    result.position = perspective(position);
    result.uv = uv;

    return result;
}

float4 psMain(PSInput input) : SV_TARGET {
    return textures[textureIndex].Sample(samplers[samplerIndex], input.uv);
}
