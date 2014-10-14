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

PixelOutput main(PixelInput input)
{
	float4 textureColor;
	float4 color;
	float3 specular;
	float4 sphereColor;
	float2 sphereUV;
	float4 toonColor;
	float4 tmp;
	float p;

	// Sample the texture pixel at this location.
	textureColor = baseTexture.Sample(SampleType, input.tex);

	tmp = textureColor * materials[input.material].mulBaseCoefficient + materials[input.material].addBaseCoefficient;
	textureColor = lerp(textureColor, tmp, materials[input.material].weight);
	color = materials[input.material].ambient;

	float3x3 lightDirections = 0, lightDiffuse = 0, lightSpecular = 0, halves = 0;
	[unroll]
	for (uint i = 0; i < lightCount; i++) {
		color *= lights[i].ambient;
		lightDirections[i] = lights[i].direction.xyz;
		lightDiffuse[i] = lights[i].diffuse.xyz;
		lightSpecular[i] = lights[i].specular.xyz;
		halves[i] = normalize(input.viewDirection - lightDirections[i]);
	}

	float3 dotL = mul(-lightDirections, input.normal);
	float3 dotH = mul(halves, input.normal);
	float3 zeroL = step(0, dotL);
	float3 diffuse = zeroL * dotL;
	specular = pow(max(dotH, 0) * zeroL, materials[input.material].specular.w);

	color.xyz = saturate(color.xyz + mul(diffuse, lightDiffuse) * materials[input.material].diffuse.xyz);

	// Multiply the texture pixel and the input color to get the final result.
	if ((materials[input.material].flags & 0x08) == 0)
		color *= textureColor;

	if (color.w <= 0) discard;

	if ((materials[input.material].flags & 0x01) == 0) {
		float3 reflection = normalize(reflect(input.viewDirection, input.normal));
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

	color.xyz = saturate(color.xyz + mul(specular, lightSpecular) * materials[input.material].specular.rgb);

	if ((materials[input.material].flags & 0x04) == 0) {
		float4 toonColor = toonTexture.Sample(SampleType, float2(input.depth, dot(dotL, input.normal) * 0.5f + 0.5f));
		tmp = toonColor*materials[input.material].mulToonCoefficient + materials[input.material].addToonCoefficient;
		toonColor = lerp(toonColor, tmp, materials[input.material].weight);
		color.xyz *= toonColor.xyz;
	}

	PixelOutput output;
	output.color = color;
	output.depth = input.depth;
	return output;
}