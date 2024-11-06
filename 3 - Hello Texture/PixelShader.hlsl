struct VSOutput
{
	float4 position : SV_POSITION;
	float3 normal : NORMAL;
	float2 uv : TEXCOORD;
};

Texture2D albedo : register(t0);
Texture2D albedo2 : register(t1);
SamplerState linearSampler : register(s0);

float4 main(VSOutput input) : SV_TARGET
{
    return albedo.Sample(linearSampler, input.uv) * 0.8 + albedo2.Sample(linearSampler, input.uv) * 0.2;
}