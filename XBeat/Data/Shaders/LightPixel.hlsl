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

cbuffer LightBuffer
{
	float4 ambientColor;
	float4 diffuseColor;
	float3 lightDirection;
	float specularPower;
	float4 specularColor;
};

cbuffer MaterialBuffer
{
	int matFlags;
	float4 matAmbient;
	float4 matDiffuse;
	float4 matSpecular;
	float3 matPadding;
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
	float p;
	float depth;

	depth = input.depth;

	// Sample the texture pixel at this location.
	textureColor = baseTexture.Sample(SampleType, input.tex);

	color.xyz = ambientColor.xyz * matAmbient.xyz;
	color.w = 1.0f;

	specular = float4(0.0f, 0.0f, 0.0f, 0.0f);
	
	// Invert the light direction for calculations.
	lightDir = -lightDirection;

	// Calculate the amount of light on this pixel.
	lightIntensity = saturate(dot(lightDir, input.normal));

	if (lightIntensity > 0.0f) {
		lightColor = saturate(color + (diffuseColor * lightIntensity));

		reflection = normalize(2 * lightIntensity * input.normal - lightDir);

		if (specularPower > 0.0f)
			specular = pow(saturate(dot(reflection, input.viewDirection)), specularPower);
	}
	else lightColor = color;
	//lightColor = ambientColor * 1.2 * lightIntensity;
	//specular = specularColor * matSpecular * lightColor * pow(max(0, dot(input.normal, normalize(lightDir + input.viewDirection))), specularPower);

	// Multiply the texture pixel and the input color to get the final result.
	color = textureColor * lightColor;
	//color = lerp(color, matDiffuse * lightColor, 0.5f) * textureColor;

	if ((matFlags & 0x01) != 0 || (matFlags & 0x04) != 0) {
		reflection = normalize(input.viewDirection - 2.0f * dot(input.viewDirection, input.normal) * input.normal);
		p = 1.0f / (2.0f * sqrt(reflection.x * reflection.x + reflection.y * reflection.y + pow(reflection.z + 1, 2.0f)));

		sphereUV = reflection.xy * p + 0.5f;
	}

	if ((matFlags & 0x01) != 0) {
		sphereColor = sphereTexture.Sample(SampleType, sphereUV);

		if ((matFlags & 0x02) == 0)
			color.xyz = color.xyz * sphereColor.xyz;
		else 
			color.xyz = color.xyz * (1.0f - sphereColor.xyz);
	}

	color = color + specular;
	
	if ((matFlags & 0x04) != 0) {
		color.xyz = color.xyz * toonTexture.Sample(SampleType, float2(depth, lightIntensity));
	}

	return saturate(color);
}
