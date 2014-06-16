////////////////////////////////////////////////////////////////////////////////
// Filename: light.ps
////////////////////////////////////////////////////////////////////////////////


/////////////
// GLOBALS //
/////////////
Texture2D baseTexture : register(t0);
Texture2D sphereTexture : register(t1);
Texture2D toonTexture : register(t2);

SamplerState SampleType;

struct MatrixBuffer
{
	matrix world;
	matrix view;
	matrix projection;
	matrix wvp;
	matrix worldInverse;
};
struct LightBuffer
{
	float4 ambient;
	float4 diffuse;
	float4 specular;
	float4 direction;
	float4 position;
};

cbuffer data : register (cb0)
{
	MatrixBuffer matrices;
	LightBuffer lights[3];

	float3 eyePosition;
	float ___padding;
	uint1 lightCount;
};

struct MaterialBufferType
{
	float4 ambient;
	float4 diffuse;
	float4 specular;
};

cbuffer MaterialBuffer : register (cb1)
{
	MaterialBufferType material;
};


//////////////
// TYPEDEFS //
//////////////
struct PixelInputType
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
	float3 normal : NORMAL;
	float3 viewDirection : TEXCOORD1;
	float depth : TEXCOORD2;
};

////////////////////////////////////////////////////////////////////////////////
// Pixel Shader
////////////////////////////////////////////////////////////////////////////////
float4 main(PixelInputType input) : SV_TARGET
{
	float4 textureColor;
	float3 lightDir;
	float lightIntensity;
	float4 color;
	float3 reflection;
	float3 specular;
	float4 sphereColor;
	float2 sphereUV;
	float4 lightColor;
	float4 toonColor;
	float4 tmp;
	float p;

	//if (material.mulBaseCoefficient.w == 0.0f) discard;

	// Sample the texture pixel at this location.
	textureColor = baseTexture.Sample(SampleType, input.tex) * material.ambient;

	for (uint i = 0; i < lightCount; i++) {
		color = lights[i].ambient;

		specular = float3(0.0f, 0.0f, 0.0f);

		// Invert the light direction for calculations.
		lightDir = -lights[i].direction.xyz;

		// Calculate the amount of light on this pixel.
		lightIntensity = saturate(dot(lightDir, input.normal));

		if (lightIntensity > 0.0f) {
			float4 append = lights[i].diffuse * material.diffuse * lightIntensity;
			lightColor = saturate(color + append);

			reflection = reflect(lightDir, input.normal);

			if (lights[i].specular.w > 0.0f) {
				specular = pow(saturate(dot(reflection, input.viewDirection)), lights[i].specular.w) * lights[i].specular.xyz * material.specular.xyz;
			}
		}
		else lightColor = color;

		// Multiply the texture pixel and the input color to get the final result.
		color = textureColor * lightColor;
		color.xyz = saturate(color.xyz + specular);
	}

	if (color.w <= 0) discard;

	return color;
}
