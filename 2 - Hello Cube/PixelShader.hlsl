struct VSOutput
{
	float4 position : SV_POSITION;
	float3 normal : NORMAL;
};


float4 main(VSOutput input) : SV_TARGET
{
	return abs(float4(input.normal, 1));
}