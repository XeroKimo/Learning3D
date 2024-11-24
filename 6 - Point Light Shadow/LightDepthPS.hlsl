struct VSOutput
{
    float4 position : SV_POSITION;
    float4 worldPosition : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
    float4 lightWorldPosition : Position2;
};

struct PSOutput
{
    float4 color : SV_Target;
    float depth : SV_Depth;
};

cbuffer PointLight : register(b0)
{
    float3 lightPosition;
    float3 lightColor;
};

PSOutput main(VSOutput output)
{
    PSOutput psOut;
    psOut.color = float4(1, 1, 1, 1);
    psOut.depth = length(output.worldPosition.xyz - lightPosition) / 20;
    return psOut;
}