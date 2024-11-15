struct VSOutput
{
    float4 position : SV_POSITION;
    float4 worldPosition : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
    float4 lightWorldPosition : Position2;
};

float4 main(VSOutput output) : SV_TARGET
{
	return float4(1.0f, 1.0f, 1.0f, 1.0f);
}