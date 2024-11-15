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
    float4 lightWorldPosition : Position2;
};

cbuffer CameraConstants : register(b0)
{
	float4x4 viewProj;
}

cbuffer ObjectConstants : register(b1)
{
	float4x4 objectTransform;
}

cbuffer LightConstants : register(b2)
{
    float4x4 lightMatrix;
}

VSOutput main(VSInput input) 
{
	VSOutput output;
    output.worldPosition = mul(objectTransform, float4(input.position, 1));
    output.lightWorldPosition = mul(lightMatrix, output.worldPosition);
    output.position = mul(viewProj, output.worldPosition);
    output.normal = normalize(mul(objectTransform, float4(input.normal, 0)).xyz);
    output.uv = input.uv;
	return output;
}