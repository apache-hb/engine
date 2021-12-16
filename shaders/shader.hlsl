cbuffer CameraProjection : register(b0) {
    float4x4 model;
    float4x4 view;
    float4x4 projection;
};

struct PSInput {
    float4 position : SV_POSITION;
    float4 worldpos : POSITION;
    float4 colour : COLOUR;
};

PSInput VSMain(float3 position : POSITION, float4 colour : COLOUR) {    
    PSInput result;

    float4 pos = float4(position, 1.f);
    pos *= -1.f;
    float4 worldpos = mul(pos, model);
    float4 viewpos = mul(mul(worldpos, view), projection);

    result.worldpos = worldpos;
    result.position = viewpos;
    result.colour = colour;

    return result;
}

float4 PSMain(PSInput input) : SV_TARGET {
    return input.colour;
}
