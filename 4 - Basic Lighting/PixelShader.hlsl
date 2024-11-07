struct VSOutput
{
    float4 position : SV_POSITION;
    float4 worldPosition : POSITION;
	float3 normal : NORMAL;
	float2 uv : TEXCOORD;
};

cbuffer PointLight : register(b0)
{
    float3 lightPosition;
    float3 lightColor;
};

cbuffer AmbientLight : register(b1)
{
    float ambientLight;
}

Texture2D albedo : register(t0);
Texture2D albedo2 : register(t1);
SamplerState linearSampler : register(s0);

float4 main(VSOutput input) : SV_TARGET
{
    float3 lightDirection = normalize(lightPosition - input.worldPosition.xyz);
    float3 reflection = reflect(-lightDirection, input.normal);
    float diffuse = (dot(lightDirection, input.normal));
    float specularStrength = 0.2;
    float specular = pow(max(dot(lightDirection, reflection), 0), 32) * specularStrength;
    
    return (albedo.Sample(linearSampler, input.uv) * 0.8 + albedo2.Sample(linearSampler, input.uv) * 0.2) * float4(lightColor, 1) * (ambientLight + diffuse + specular);
}