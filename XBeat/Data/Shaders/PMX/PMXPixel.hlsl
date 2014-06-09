#include "Classes.hlsli"

cbuffer data : register (cb0)
{
	MatrixBuffer matrices;
	LightBuffer lights[4];

	float3 eyePosition;
	float ___padding;
	uint lightCount;
	float3 __padding2;
};

StructuredBuffer<MaterialBufferType> materials : register (u4);
Texture2D baseTexture : register(t0);
Texture2D sphereTexture : register(t1);
Texture2D toonTexture : register(t2);
SamplerState SampleType;

float4 main(PixelInput input) : SV_TARGET
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

	// Sample the texture pixel at this location.
	textureColor = baseTexture.Sample(SampleType, input.tex);

	tmp = textureColor * materials[input.material].mulBaseCoefficient + materials[input.material].addBaseCoefficient;
	textureColor = lerp(textureColor, tmp, materials[input.material].weight);
	color = materials[input.material].ambient;

	lightColor = float4(0.0f, 0.0f, 0.0f, 0.0f);
	for (uint i = 0; i < lightCount; i++) {
		color *= lights[i].ambient;
		tmp = float4(0.0f, 0.0f, 0.0f, 0.0f);
		specular = float4(0.0f, 0.0f, 0.0f, 0.0f);

		// Invert the light direction for calculations.
		lightDir = -lights[i].direction.xyz;

		// Calculate the amount of light on this pixel.
		lightIntensity = saturate(dot(lightDir, input.normal));

		if (lightIntensity > 0.0f) {
			float4 append = lights[i].diffuse * lightIntensity * materials[input.material].diffuse;
			lightColor = saturate(lightColor + color + append);

			reflection = reflect(lightDir, input.normal);

			if (lights[i].specular.w > 0.0f) {
				specular = float4(pow(saturate(dot(reflection, input.viewDirection)), lights[i].specular.w) * lights[i].specular.xyz * materials[input.material].specular.xyz, 0.0f);
			}
		}
	}

	// Multiply the texture pixel and the input color to get the final result.
	if ((materials[input.material].flags & 0x08) == 0)
		color = textureColor * lightColor;
	else
		color = lightColor;

	if (color.w <= 0) discard;

	if ((materials[input.material].flags & 0x01) == 0) {
		reflection = normalize(reflect(input.viewDirection, input.normal));
		p = 1.0f / (2.0f * sqrt(reflection.x * reflection.x + reflection.y * reflection.y + pow(reflection.z + 1, 2.0f)));

		sphereUV = reflection.xy * p + 0.5f;

		sphereColor = saturate(sphereTexture.Sample(SampleType, sphereUV));
		tmp = sphereColor*materials[input.material].mulSphereCoefficient + materials[input.material].addSphereCoefficient;
		sphereColor = lerp(sphereColor, tmp, materials[input.material].weight);

		if ((materials[input.material].flags & 0x02) == 0)
			color.xyz += color.xyz * sphereColor.xyz;
		else
			color.xyz += sphereColor.xyz;

		color = saturate(color);
	}

	color.xyz = saturate(color.xyz + specular.xyz);

	if ((materials[input.material].flags & 0x04) == 0) {
		float4 toonColor = toonTexture.Sample(SampleType, float2(input.depth, dot(lightDir, input.normal) * 0.5f + 0.5f));
		tmp = toonColor*materials[input.material].mulToonCoefficient + materials[input.material].addToonCoefficient;
		toonColor = lerp(toonColor, tmp, materials[input.material].weight);
		color.xyz *= toonColor.xyz;
	}

	return color;
}