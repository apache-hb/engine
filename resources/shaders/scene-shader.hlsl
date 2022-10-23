#if DEBUG
#   define DEBUG_NORMALS (1 << 0)
#   define DEBUG_UVS (1 << 1)
cbuffer DebugBuffer : register(b0, space1) {
    uint debugFlags;
};
#endif

cbuffer SceneBuffer : register(b0) {
    float4x4 view;
    float4x4 projection;
    
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
    float4 world : POSITION;

    float2 uv : TEXCOORD;

    float3 normal : NORMAL;
};

Texture2D gTextures[] : register(t0);
SamplerState gSampler : register(s0);

float4 perspective(float4 pos) {
    float4 result = mul(pos, transform);
    result = mul(result, view);
    result = mul(result, projection);
    return result;
}

PSInput vsMain(float3 pos : POSITION, float3 normal : NORMAL, float2 uv : TEXCOORD) {
    PSInput result;
    result.pos = perspective(float4(pos, 1.f));
    result.world = mul(float4(pos, 1.f), transform);
    result.normal = normal;
    result.uv = uv;
    return result;
}

float4 psMain(PSInput input) : SV_TARGET {
#if DEBUG
    if (debugFlags & DEBUG_NORMALS) {
        return float4(input.normal.xyz, 1.f);
    } else if (debugFlags & DEBUG_UVS) {
        return float4(input.uv, 0.f, 1.f);
    }
#endif

    // diffuse lighting
    float3 lightDir = normalize(light - input.world.xyz);
    float3 normal = normalize(input.normal.xyz);
    float diffuse = max(0.f, dot(normal, lightDir));

    // texture
    float4 colour = gTextures[texture].Sample(gSampler, input.uv);

    return colour * diffuse;
}
