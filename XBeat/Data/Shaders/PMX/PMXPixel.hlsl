#include "Classes.hlsli"

cbuffer data : register (cb0)
{
	MatrixBuffer matrices;
	LightBuffer lights[MAX_LIGHTS];

	float3 eyePosition;
	float ___padding;
	uint lightCount;
	float3 __padding2;
};

Texture2D baseTexture : register(t0);
Texture2D sphereTexture : register(t1);
Texture2D toonTexture : register(t2);
StructuredBuffer<MaterialBufferType> materials : register(t4);
SamplerState SampleType;

struct LightingOutput
{
	float3 Diffuse;
	float3 Specular;
	float3 DotL;
};

LightingOutput ComputeLights(float3 eyeVector, float3 worldNormal, uint numLights, float3 DiffuseColor, float3 SpecularColor, float SpecularPower)
{
	float3x3 lightDirections = 0;
	float3x3 lightDiffuse = 0;
	float3x3 lightSpecular = 0;
	float3x3 halfVectors = 0;

	[unroll]
	for (uint i = 0; i < numLights; i++)
	{
		lightDirections[i] = lights[i].direction.xyz;
		lightDiffuse[i] = lights[i].diffuse.xyz;
		lightSpecular[i] = lights[i].specular.xyz;

		halfVectors[i] = normalize(eyeVector - lightDirections[i]);
	}

	float3 dotL = mul(-lightDirections, worldNormal);
	float3 dotH = mul(halfVectors, worldNormal);

	float3 zeroL = step(0, dotL);

	float3 diffuse = zeroL * dotL;
	float3 specular = pow(max(dotH, 0) * zeroL, SpecularPower);

	LightingOutput result;

	result.Diffuse = mul(diffuse, lightDiffuse)  * DiffuseColor;
	result.Specular = mul(specular, lightSpecular) * SpecularColor;
	result.DotL = dotL;

	return result;
}

PixelOutput main(PixelInput input)
{
	float4 textureColor;
	float4 color;
	float3 specular;
	float4 sphereColor;
	float2 sphereUV;
	float4 toonColor;
	float p;

	// Sample the texture pixel at this location.
	textureColor = baseTexture.Sample(SampleType, input.tex);

	textureColor = lerp(textureColor, textureColor, materials[input.material].weight);
	color = materials[input.material].ambient;

	LightingOutput lighting = ComputeLights(input.viewDirection, -input.normal, lightCount, materials[input.material].diffuse.rgb, materials[input.material].specular.rgb, materials[input.material].specular.a);

	color.rgb += max(lighting.Diffuse, float3(0.25f, 0.25f, 0.25f));

	// Multiply the texture pixel and the input color to get the final result.
	if ((materials[input.material].flags & 0x08) == 0)
		color *= textureColor;

	color = color * materials[input.material].mulBaseCoefficient + materials[input.material].addBaseCoefficient;
	color.a *= materials[input.material].diffuse.a;

	if (color.a <= 0) discard;

	if ((materials[input.material].flags & 0x01) == 0) {
		float3 reflection = normalize(reflect(input.viewDirection, input.normal));
		p = 1.0f / (2.0f * sqrt(reflection.x * reflection.x + reflection.y * reflection.y + pow(reflection.z + 1, 2.0f)));

		sphereUV = reflection.xy * p + 0.5f;

		sphereColor = sphereTexture.Sample(SampleType, sphereUV);
		float4 tmp = sphereColor*materials[input.material].mulSphereCoefficient + materials[input.material].addSphereCoefficient;
		sphereColor = lerp(sphereColor, tmp, materials[input.material].weight);

		if ((materials[input.material].flags & 0x02) == 0)
			color.rgb += color.rgb * sphereColor.rgb;
		else
			color.rgb += sphereColor.rgb;
	}

	color.rgb += lighting.Specular;

	if ((materials[input.material].flags & 0x04) == 0) {
		float4 toonColor = toonTexture.Sample(SampleType, float2(input.depth, dot(lighting.DotL, input.normal) * 0.5f + 0.5f));
		float4 tmp = toonColor*materials[input.material].mulToonCoefficient + materials[input.material].addToonCoefficient;
		toonColor = lerp(toonColor, tmp, materials[input.material].weight);
		color.xyz *= toonColor.xyz;
	}

	PixelOutput output;
	output.color = saturate(color);
	output.depth = input.depth;
	return output;
}