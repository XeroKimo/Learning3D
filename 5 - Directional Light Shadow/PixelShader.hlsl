struct VSOutput
{
    float4 position : SV_POSITION;
    float4 worldPosition : POSITION;
	float3 normal : NORMAL;
	float2 uv : TEXCOORD;
    float4 lightWorldPosition : Position2;
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

Texture2D shadowMap : register(t2);

float ShadowCalculation(float4 position)
{
    float3 projectionCoordinates = position.xyz / position.w;
    //Converting coordinates to NDC space
    projectionCoordinates = projectionCoordinates * 0.5 + float3(0.5, 0.5, 0.5);
    //Sampling coordinates when compared to OpenGL is flipped on the y axis
    projectionCoordinates.y = 1 - projectionCoordinates.y;
    float closestDepth = shadowMap.Sample(linearSampler, projectionCoordinates.xy).x;
    
    //The projection matrices used in this codebase already makes depth stay in NDC space so we don't need the projection coordinates values
    float currentDepth = projectionCoordinates.z;
    
    return currentDepth > closestDepth ? 1 : 0;
}

float4 main(VSOutput input) : SV_TARGET
{
    float3 lightDirection = normalize(lightPosition - input.worldPosition.xyz);
    float3 reflection = reflect(-lightDirection, input.normal);
    float diffuse = (dot(lightDirection, input.normal));
    float specularStrength = 0.5;
    float specular = pow(max(dot(lightDirection, reflection), 0), 32) * specularStrength;
    
    return (albedo.Sample(linearSampler, input.uv) * 0.8 + albedo2.Sample(linearSampler, input.uv) * 0.2) * float4(lightColor, 1) * (ambientLight + (1 - ShadowCalculation(input.lightWorldPosition)) * (diffuse + specular));
}