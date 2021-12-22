Texture2D<float4> src : register(t0);
RWTexture2D<float4> dst : register(u0);
SamplerState texSampler : register(s0);

cbuffer ConstBuffer : register(b0) {
    float2 texelSize; // = 1.f / dst dimensions
};

[numthreads(8, 8, 1)]
void genMipMaps(uint3 thread : SV_DispatchThreadID) {
    float2 coords = texelSize * (thread.xy + 0.5);
    float4 colour = src.SampleLevel(texSampler, coords, 0);
    dst[thread.xy] = colour;
}
