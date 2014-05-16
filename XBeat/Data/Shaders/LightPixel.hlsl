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

cbuffer LightBuffer : register (cb0)
{
	float4 ambientColor;
	float4 diffuseColor;
	float3 lightDirection;
	float specularPower;
	float4 specularColor;
};

struct MaterialBufferType
{
	float4 ambient;
	float4 diffuse;
	float4 specular;
	float4 mulBaseCoefficient;
	float4 mulSphereCoefficient;
	float4 mulToonCoefficient;
	float4 addBaseCoefficient;
	float4 addSphereCoefficient;
	float4 addToonCoefficient;

	unsigned int flags;
	float weight;
	int index;
	int padding;
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
	float4 specular;
	float4 sphereColor;
	float2 sphereUV;
	float4 lightColor;
	float4 toonColor;
	float4 tmp;
	float p;

	//if (material.mulBaseCoefficient.w == 0.0f) discard;

	// Sample the texture pixel at this location.
	textureColor = baseTexture.Sample(SampleType, input.tex);
	color = ambientColor;

	if (material.index != -1) {
		tmp = textureColor * material.mulBaseCoefficient + material.addBaseCoefficient;
		textureColor = lerp(textureColor, tmp, material.weight);
		color *= material.ambient;
	}

	specular = float4(0.0f, 0.0f, 0.0f, 0.0f);
	
	// Invert the light direction for calculations.
	lightDir = -lightDirection;

	// Calculate the amount of light on this pixel.
	lightIntensity = saturate(dot(lightDir, input.normal));
	
	if (lightIntensity > 0.0f) {
		float4 append = diffuseColor * lightIntensity;
		if (material.index != -1)
			append *= material.diffuse;
		lightColor = saturate(color + append);

		reflection = reflect(lightDir, input.normal);

		if (specularPower > 0.0f) {
			specular = pow(saturate(dot(reflection, input.viewDirection)), specularPower) * specularColor;
			if (material.index != -1) {
				specular *= material.specular;
			}
		}
	}
	else lightColor = color;

	// Multiply the texture pixel and the input color to get the final result.
	if ((material.flags & 0x08) == 0)
		color = textureColor * lightColor;
	else
		color = lightColor;

	if (color.w <= 0) discard;

	if (material.index == -1) {
		color.xyz = saturate(color.xyz + specular.xyz);
		return color;
	}

	if ((material.flags & 0x01) == 0) {
		reflection = normalize(reflect(input.viewDirection, input.normal));
		p = 1.0f / (2.0f * sqrt(reflection.x * reflection.x + reflection.y * reflection.y + pow(reflection.z + 1, 2.0f)));

		sphereUV = reflection.xy * p + 0.5f;

		sphereColor = saturate(sphereTexture.Sample(SampleType, sphereUV));
		tmp = sphereColor*material.mulSphereCoefficient + material.addSphereCoefficient;
		sphereColor = lerp(sphereColor, tmp, material.weight);

		if ((material.flags & 0x02) == 0)
			color.xyz += color.xyz * sphereColor.xyz;
		else
			color.xyz += sphereColor.xyz;

		color = saturate(color);
	}

	color.xyz = saturate(color.xyz + specular.xyz);
	
	if ((material.flags & 0x04) == 0) {
		float4 toonColor = toonTexture.Sample(SampleType, float2(input.depth, dot(lightDir, input.normal) * 0.5f + 0.5f));
		tmp = toonColor*material.mulToonCoefficient + material.addToonCoefficient;
		toonColor = lerp(toonColor, tmp, material.weight);
		color.xyz *= toonColor.xyz;
	}

	return color;
}
