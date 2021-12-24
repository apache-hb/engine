Texture2D sceneTexture : register(t0);
SamplerState sceneSampler : register(s0);

struct PSInput {
    float4 position : SV_POSITION;
    float2 coords : TEXCOORD;
};

PSInput vsMain(float4 position : POSITION, float2 coords : TEXCOORD) {
    PSInput result;
    result.position = position;
    result.coords = coords;
    return result;
}

float4 psMain(PSInput input) : SV_TARGET {
    return sceneTexture.Sample(sceneSampler, input.coords);
}
