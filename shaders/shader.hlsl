cbuffer ConstBuffer : register(b0) {
    float4x4 model;
    float4x4 view;
    float4x4 projection;

    float3 offset;
};

struct PSInput {
    float4 position : SV_POSITION;
    float4 colour : COLOUR;
};

PSInput VSMain(float3 position : POSITION, float4 colour : COLOUR) {    
    PSInput result;

    float4 pos = float4(position, 1.f);

    pos = mul(pos, model);
    pos = mul(pos, view);
    pos = mul(pos, projection);

    result.position = pos;
    result.colour = colour;

    return result;
}

float4 PSMain(PSInput input) : SV_TARGET {
    return input.colour;
}
