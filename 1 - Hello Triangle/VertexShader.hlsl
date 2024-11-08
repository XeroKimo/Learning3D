struct VSInput
{
	float3 position : POSITION;
	float4 color : COLOR;
};

struct VSOutput
{
	float4 position : SV_POSITION;
	float4 color : COLOR;
};

VSOutput main(VSInput input) 
{
	VSOutput output;
	output.position = float4(input.position, 1);
	output.color = input.color;
	return output;
}