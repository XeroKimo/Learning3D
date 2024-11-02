struct VSInput
{
	float3 position : POSITION;
	float3 normal : NORMAL;
};

struct VSOutput
{
	float4 position : SV_POSITION;
	float3 normal : NORMAL;
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
	output.position = mul(objectTransform, float4(input.position, 1));
	output.position = mul(viewProj, output.position);
	output.normal = input.normal;
	return output;
}