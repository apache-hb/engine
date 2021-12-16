struct PSInput {
    float4 position : SV_POSITION;
    float4 colour : COLOUR;
};

PSInput VSMain(float3 position : POSITION, float4 colour : COLOUR) {    
    PSInput result;

    result.position = float4(position, 1.f);
    result.colour = colour;

    return result;
}

float4 PSMain(PSInput input) : SV_TARGET {
    return input.colour;
}
