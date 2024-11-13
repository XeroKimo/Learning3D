
struct VSOutput
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
};

Texture2D albedo : register(t0);
SamplerState linearSampler : register(s0);

float LinearizeDepth(float depth)
{
    //return depth;
    float nearPlane = 0.1f;
    float farPlane = 10.f;
    float z = depth * 2 - 1;
    return ((2 * nearPlane * farPlane) / (farPlane + nearPlane - z * (farPlane - nearPlane))) / farPlane;
}

float4 main(VSOutput input) : SV_TARGET
{
    float depth = LinearizeDepth(albedo.Sample(linearSampler, input.uv).x);
    return float4(depth, depth, depth, 1.0f);
}