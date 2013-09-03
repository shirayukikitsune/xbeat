// Define our types here
struct VertexInputType
{
	float4 position : POSITION;
	float2 texCoord : TEXCOORD0;
};

struct PixelInputType
{
	float4 position : SV_POSITION;
	float2 texCoord : TEXCOORD0;
	float2 texelSize : TEXCOORD1;
};

// Define our buffers, variables and constants here
SamplerState TexSampler : register (s0);
Texture2D Scenes[3];

cbuffer WVP : register (cb0) {
	matrix world;
	matrix view;
	matrix projection;
};

cbuffer ScreenSize : register (cb1) {
	float2 screenDimentions;
	float2 texelSize;
};

float blurDistance = 0.002f;
float blurWeights[9] = { 0.00f, 0.02f, 0.10f, 0.23f, 0.30f, 0.23f, 0.10f, 0.02f, 0.00f };

float4 adjustSaturation(float4 color, float saturation)
{
	float grey = dot(color, float4(0.3, 0.59, 0.11, 1.0f));

	return lerp(grey, color, saturation);
}

PixelInputType mainvs(VertexInputType input)
{
	PixelInputType output;
	float2 pos;

	pos = sign(input.position.xy);

	output.position = float4(pos.x, pos.y, 0.0f, 1.0f);
	/*output.position = mul(output.position, world);
	output.position = mul(output.position, view);
	output.position = mul(output.position, projection);*/
	output.texCoord = input.texCoord;
	output.texelSize = float2(1.0f / 1024.0f, 1.0f / 1024.0f);

	return output;
}

float4 mainps(PixelInputType input) : SV_TARGET
{
	float4 texColor;

	texColor = Scenes[0].Sample(TexSampler, input.texCoord);

	return texColor;
}

float4 horblurps(PixelInputType input) : SV_TARGET
{
	// Make an average of surrounding pixels
	float4 sum = float4(0.0f, 0.0f, 0.0f, 0.0f);
	float2 uv = float2(input.texCoord.x - 4.0f * blurDistance, input.texCoord.y);
	int i;

	for (i = 0; i < 9; i++) {
		uv.x += blurDistance;
		sum += Scenes[2].Sample(TexSampler, uv) * blurWeights[i];
	}

	return sum;
}

float4 verblurps(PixelInputType input) : SV_TARGET
{
	// Make an average of surrounding pixels
	float4 sum = float4(0.0f, 0.0f, 0.0f, 0.0f);
	
	sum += Scenes[2].Sample(TexSampler, float2(input.texCoord.x, input.texCoord.y - 4.0f * blurDistance)) * blurWeights[0];
	sum += Scenes[2].Sample(TexSampler, float2(input.texCoord.x, input.texCoord.y - 3.0f * blurDistance)) * blurWeights[1];
	sum += Scenes[2].Sample(TexSampler, float2(input.texCoord.x, input.texCoord.y - 2.0f * blurDistance)) * blurWeights[2];
	sum += Scenes[2].Sample(TexSampler, float2(input.texCoord.x, input.texCoord.y - blurDistance)) * blurWeights[3];
	sum += Scenes[2].Sample(TexSampler, float2(input.texCoord.x, input.texCoord.y)) * blurWeights[4];
	sum += Scenes[2].Sample(TexSampler, float2(input.texCoord.x, input.texCoord.y + blurDistance)) * blurWeights[5];
	sum += Scenes[2].Sample(TexSampler, float2(input.texCoord.x, input.texCoord.y + 2.0f * blurDistance)) * blurWeights[6];
	sum += Scenes[2].Sample(TexSampler, float2(input.texCoord.x, input.texCoord.y + 3.0f * blurDistance)) * blurWeights[7];
	sum += Scenes[2].Sample(TexSampler, float2(input.texCoord.x, input.texCoord.y + 4.0f * blurDistance)) * blurWeights[8];

	return sum;
}

// Depth of Field
cbuffer dofbuffer : register (cb0)
{
	float distance;
	float range;
	float near;
	float far;
};

float4 dofps(PixelInputType input) : SV_TARGET
{
	float4 blurColor = Scenes[2].Sample(TexSampler, input.texCoord);
	float4 originalColor = Scenes[0].Sample(TexSampler, input.texCoord);

	// Get the inverted depth texel, meaning that nearer the object is, darker the scene
	float depth = 1 - Scenes[1].Sample(TexSampler, input.texCoord);

	float sceneZ = (-near * far) / (depth - far);
	float factor = saturate(abs(sceneZ - distance) / range);

	return lerp(originalColor, blurColor, factor);
}

// Bloom pass constants, can be passed via a constant buffer (cbuffer)
float bloomSaturation = 1.0f;
float bloomIntensity = 1.1f;
float originalSaturation = 1.0f;
float originalIntensity = 0.8f;

float4 bloomps(PixelInputType input) : SV_TARGET
{
	float4 bloomColor = Scenes[2].Sample(TexSampler, input.texCoord);
	float4 originalColor = Scenes[0].Sample(TexSampler, input.texCoord);

	bloomColor = adjustSaturation(bloomColor, bloomSaturation) * bloomIntensity;
	originalColor = adjustSaturation(originalColor, originalSaturation) * originalIntensity;

	originalColor *= 1 - saturate(bloomColor);

	return originalColor + bloomColor;
}


// Define our technique here
//   The bloom needs to blur the screen first (we blur it twice, once horizontally, another vertically), then applies the effect
technique11 Bloom
{
	pass P0 // Just vertex shader
	{
		VertexShader = compile vs_5_0 mainvs();
		PixelShader = compile ps_5_0 mainps();
	}
	pass horizontalgaussianblur
	{
		PixelShader = compile ps_5_0 horblurps();
	}
	pass verticalgaussianblur
	{
		PixelShader = compile ps_5_0 verblurps();
	}
	/*pass dof
	{
		PixelShader = compile ps_5_0 dofps();
	}*/
	pass bloom
	{
		PixelShader = compile ps_5_0 bloomps();
	}
}
