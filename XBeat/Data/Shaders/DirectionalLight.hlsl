cbuffer MatrixBuffer
{
    matrix worldMatrix;
    matrix viewMatrix;
    matrix projectionMatrix;
};

cbuffer CameraBuffer
{
	float3 cameraPosition;
	float padding;
};

struct VertexInputType
{
    float4 position : POSITION;
    float2 tex : TEXCOORD0;
	float3 normal : NORMAL;
};

struct PixelInputType
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
	float3 normal : NORMAL;
	float3 viewDirection : TEXCOORD1;
};

PixelInputType vsmain(VertexInputType input)
{
    PixelInputType output;
    float4 worldPosition;

    // Change the position vector to be 4 units for proper matrix calculations.
    input.position.w = 1.0f;

    // Calculate the position of the vertex against the world, view, and projection matrices.
    output.position = mul(input.position, worldMatrix);
    output.position = mul(output.position, viewMatrix);
    output.position = mul(output.position, projectionMatrix);

    // Store the texture coordinates for the pixel shader.
    output.tex = input.tex;

	output.normal = normalize(mul(input.normal, (float3x3)worldMatrix));

	worldPosition = mul(input.position, worldMatrix);

	output.viewDirection = normalize(cameraPosition.xyz - worldPosition.xyz);
    
    return output;
}

cbuffer LightBuffer {
	float4 ambientColor;
	float4 diffuseColor;
	float3 lightDirection;
	float specularPower;
	float4 specularColor;
};

Texture2D shaderTextures[3];
SamplerState SampleType;

float4 psmain(PixelInputType input) : SV_TARGET
{
    float4 texture1Color;
    float4 texture2Color;
    float4 texture3Color;
	float3 lightDir;
	float4 color;
	float lightIntensity;
	float3 reflection;
	float4 specular;

    // Sample the pixel color from the texture using the sampler at this texture coordinate location.
	texture1Color = shaderTextures[0].Sample(SampleType, input.tex);

	color = ambientColor;

	specular = float4(0.0f, 0.0f, 0.0f, 0.0f);

	lightDir = -lightDirection;
	lightIntensity = saturate(dot(input.normal, lightDir));

	if (lightIntensity > 0.0f) {
		color += diffuseColor * lightIntensity;
		color = saturate(color);

		reflection = normalize(2 * lightIntensity * input.normal - lightDir);

		specular = pow(saturate(dot(reflection, input.viewDirection)), specularPower);
	}

	color = color * texture1Color;
	
	color = saturate(color + specular);

    return color;
}
