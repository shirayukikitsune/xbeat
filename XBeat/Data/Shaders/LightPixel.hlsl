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
	float2 padding;
};

cbuffer MaterialBuffer : register (cb1)
{
	MaterialBufferType materials[200];
};

cbuffer MaterialInfoBuffer : register (cb2)
{
	uint matIdx;
	float3 matinfopadding;
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

	//if (matMulBaseCoefficient.w == 0.0f) discard;

	// Sample the texture pixel at this location.
	textureColor = baseTexture.Sample(SampleType, input.tex);
	color = ambientColor;

	if (matIdx != -1) {
		tmp = textureColor * materials[matIdx].mulBaseCoefficient + materials[matIdx].addBaseCoefficient;
		textureColor = lerp(textureColor, tmp, materials[matIdx].weight);
		color *= materials[matIdx].ambient;
	}

	specular = float4(0.0f, 0.0f, 0.0f, 0.0f);
	
	// Invert the light direction for calculations.
	lightDir = -lightDirection;

	// Calculate the amount of light on this pixel.
	lightIntensity = saturate(dot(lightDir, input.normal));
	
	if (lightIntensity > 0.0f) {
		float4 append = diffuseColor * lightIntensity;
		if (matIdx != -1)
			append *= materials[matIdx].diffuse;
		lightColor = saturate(color + append);

		reflection = reflect(lightDir, input.normal);

		if (specularPower > 0.0f) {
			specular = pow(saturate(dot(reflection, input.viewDirection)), specularPower) * specularColor;
			if (matIdx != -1) {
				specular *= materials[matIdx].specular;
			}
		}
	}
	else lightColor = color;

	// Multiply the texture pixel and the input color to get the final result.
	if ((materials[matIdx].flags & 0x08) == 0)
		color = textureColor * lightColor;
	else
		color = lightColor;

	if (matIdx == -1) {
		color.xyz = saturate(color.xyz + specular.xyz);
		return color;
	}

	if ((materials[matIdx].flags & 0x01) == 0) {
		reflection = normalize(reflect(input.viewDirection, input.normal));
		p = 1.0f / (2.0f * sqrt(reflection.x * reflection.x + reflection.y * reflection.y + pow(reflection.z + 1, 2.0f)));

		sphereUV = reflection.xy * p + 0.5f;

		sphereColor = saturate(sphereTexture.Sample(SampleType, sphereUV));
		tmp = sphereColor*materials[matIdx].mulSphereCoefficient + materials[matIdx].addSphereCoefficient;
		sphereColor = lerp(sphereColor, tmp, materials[matIdx].weight);

		if ((materials[matIdx].flags & 0x02) == 0)
			color.xyz += color.xyz * sphereColor.xyz;
		else
			color.xyz += sphereColor.xyz;

		color = saturate(color);
	}

	color.xyz = saturate(color.xyz + specular.xyz);
	
	if ((materials[matIdx].flags & 0x04) == 0) {
		float4 toonColor = toonTexture.Sample(SampleType, float2(input.depth, dot(lightDir, input.normal) * 0.5f + 0.5f));
		tmp = toonColor*materials[matIdx].mulToonCoefficient + materials[matIdx].addToonCoefficient;
		toonColor = lerp(toonColor, tmp, materials[matIdx].weight);
		color.xyz *= toonColor.xyz;
	}

	return color;
}
