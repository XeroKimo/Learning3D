struct VSInput
{
	float3 position : POSITION;
	float3 normal : NORMAL;
	float2 uv : TEXCOORD;
};

struct VSOutput
{
	float4 position : SV_POSITION;
	float4 worldPosition : POSITION;
	float3 normal : NORMAL;
	float2 uv : TEXCOORD;
};

cbuffer CameraConstants : register(b0)
{
	float4x4 viewProj;
}

cbuffer ObjectConstants : register(b1)
{
	float4x4 objectTransform;
}

VSOutput main(VSInput input) 
{
	VSOutput output;
    output.worldPosition = mul(objectTransform, float4(input.position, 1));
    output.position = mul(viewProj, output.worldPosition);
    output.normal = normalize(mul(objectTransform, float4(input.normal, 0)).xyz);
    output.uv = input.uv;
	return output;
}