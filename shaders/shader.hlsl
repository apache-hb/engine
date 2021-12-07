cbuffer ConstBuffer : register(b0) {
    float4 offset;
};

struct PSInput {
    float4 position : SV_POSITION;
    float4 colour : COLOUR;
};

PSInput VSMain(float4 position : POSITION, float4 colour : COLOUR) {
    PSInput result;
    
    result.position = position + offset;
    result.colour = colour;

    return result;
}

float4 PSMain(PSInput input) : SV_TARGET {
    return input.colour;
}
