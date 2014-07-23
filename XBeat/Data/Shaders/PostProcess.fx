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
};

// Define our buffers, variables and constants here
SamplerState TexSampler;
SamplerState DepthSampler {
	Filter = MIN_MAG_MIP_POINT;
	AddressU = Clamp;
	AddressV = Clamp;
};
SamplerState BlurSampler {
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = Clamp;
	AddressV = Clamp;
};
Texture2D OriginalScene;
Texture2D DepthScene;
Texture2D CurrentScene;

cbuffer WVP {
	matrix world;
	matrix view;
	matrix projection;
};

cbuffer ScreenSize {
	float2 screenDimentions;
	float2 texelSize;
};

static const float NearBlurDepth = 0.1;
//The difference between the focal plane and the near plane depths.
static const float FarBlurDepth = 0.1;
//The difference between the focal plane and the far plane depths.
static const float MaxBlurCutoff = 1;
//The maximum blurriness that an object behind the focal plane can have, where 1 is full blur and 0 is none.
 
static const float dofMinThreshold = 0.1; //0.5;
//Ensures a smoother transition between near focal plane and focused area.
 
static const float ApertureDiameter = 3;

#define BLUR_SAMPLE_COUNT 15
cbuffer GaussianBlurBuffer {
	float2 blurWeightsAndOffsets[BLUR_SAMPLE_COUNT];
}

static const float2 poisson[8] =
{
       0.0,    0.0,
     0.527, -0.085,
    -0.040,  0.536,
    -0.670, -0.179,
    -0.419, -0.616,
     0.440, -0.639,
    -0.757,  0.349,
     0.574,  0.685,
};

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
	output.texCoord = input.texCoord;

	return output;
}

float4 horblurps(PixelInputType input) : SV_TARGET
{
	// Make an average of surrounding pixels
	float4 sum = float4(0.0f, 0.0f, 0.0f, 0.0f);

	for (int i = 0; i < BLUR_SAMPLE_COUNT; i++)
		sum += CurrentScene.Sample(BlurSampler, float2(input.texCoord.x + blurWeightsAndOffsets[i].x * texelSize.x, input.texCoord.y)) * blurWeightsAndOffsets[i].y;

	sum.w = 1.0f;

	return sum;
}

float4 verblurps(PixelInputType input) : SV_TARGET
{
	// Make an average of surrounding pixels
	float4 sum = float4(0.0f, 0.0f, 0.0f, 0.0f);

	for (int i = 0; i < BLUR_SAMPLE_COUNT; i++)
		sum += CurrentScene.Sample(BlurSampler, float2(input.texCoord.x, input.texCoord.y + blurWeightsAndOffsets[i].x * texelSize.y)) * blurWeightsAndOffsets[i].y;

	sum.w = 1.0f;

	return sum;
}

// Depth of Field
cbuffer dofbuffer
{
	float near;
	float far;
	float range;
	float dofunused;
};

// Bloom pass constants, can be passed via a constant buffer (cbuffer)
float bloomSaturation = 1.0f;
float bloomIntensity = 1.1f;
float originalSaturation = 1.0f;
float originalIntensity = 0.9f;

float4 bloomps(PixelInputType input) : SV_TARGET
{
	float4 bloomColor = CurrentScene.Sample(TexSampler, input.texCoord);
	float4 originalColor = OriginalScene.Sample(TexSampler, input.texCoord);

	bloomColor = adjustSaturation(bloomColor, bloomSaturation) * bloomIntensity;
	originalColor = adjustSaturation(originalColor, originalSaturation) * originalIntensity;

	originalColor *= 1 - saturate(bloomColor);

	return originalColor + bloomColor;
}

float GetDepthBluriness(float fDepth, float4 dofParamsFocus) {
	// 0 - in focus, 1 - completely out of focus
	float f = abs(fDepth - dofParamsFocus.z);
 
	// point is closer than focus plane.
	if (fDepth < dofParamsFocus.z) {
		f /= dofParamsFocus.x;
	}
	// point is futher than focus plane.
	else {
		f /= dofParamsFocus.y;
		f = clamp(f, 0, 1 - dofMinThreshold);
	}
 
	if (fDepth < 0.005)
		f = 0.0f;
 
	return f;
}
 
float4 GetDepth(PixelInputType input) : SV_TARGET {
	float3 color = CurrentScene.Sample(TexSampler, input.texCoord).rgb;
 
	// OBGEv2 Depth (HawkleyFox):
	float depth = pow(abs(DepthScene.Sample(DepthSampler,input.texCoord).x), 0.05);
	depth = (2.0f * near) / (near + far - depth * (far - near));
 
	// Focus depth (HawkleyFox):
	float fdepth = 0.0f;
#ifndef	DISTANTBLUR
	fdepth = pow(abs(DepthScene.Sample(DepthSampler, float2(0.5, 0.5)).x), 0.05);
#endif
	fdepth = (2.0f * near) / (near + far - fdepth * (far - near));
 
	float4 dofParamsFocus = float4(NearBlurDepth, FarBlurDepth, fdepth, MaxBlurCutoff);
	depth = saturate(GetDepthBluriness(depth, dofParamsFocus)) * dofParamsFocus.w;
 
	return float4(color, depth);
}
 
float4 DofPS(PixelInputType input) : SV_TARGET {
	// fetch center tap from blured low res image
	float centerDepth = CurrentScene.Sample(TexSampler, input.texCoord).w; 	//Gets relative depth with blur amount.
	float cdepth = DepthScene.Sample(DepthSampler, input.texCoord).x; 		//Gets absolute depth.
 
	// disc radius is circle of confusion - how much blur an object (or pixel) has.
	// it is given by c=A*abs(S2-S1)/S2
	// where A is aperture size, S2 is the distance to an out of focus object,
	// and S1 is the distance to the focused object.
 
	float discRadius = ApertureDiameter * centerDepth / cdepth;
	// or is it: discRadius = ApertureDiameter * centerDepth; ?
 
//	float discRadiusLow = discRadius * radiusScale;
	float4 cOut = 0;
 
	for (int t = 0; t < 8; t++)
		cOut += CurrentScene.Sample(TexSampler, input.texCoord + poisson[t] * texelSize * discRadius);
 
	cOut = cOut / 8;
 
	return float4(cOut.rgb, 1);
}


static const float3x3 G2[9] = {
	1.0 / (2.0*sqrt(2.0)) * float3x3(1.0, sqrt(2.0), 1.0, 0.0, 0.0, 0.0, -1.0, -sqrt(2.0), -1.0),
	1.0 / (2.0*sqrt(2.0)) * float3x3(1.0, 0.0, -1.0, sqrt(2.0), 0.0, -sqrt(2.0), 1.0, 0.0, -1.0),
	1.0 / (2.0*sqrt(2.0)) * float3x3(0.0, -1.0, sqrt(2.0), 1.0, 0.0, -1.0, -sqrt(2.0), 1.0, 0.0),
	1.0 / (2.0*sqrt(2.0)) * float3x3(sqrt(2.0), -1.0, 0.0, -1.0, 0.0, 1.0, 0.0, 1.0, -sqrt(2.0)),
	1.0 / 2.0 * float3x3(0.0, 1.0, 0.0, -1.0, 0.0, -1.0, 0.0, 1.0, 0.0),
	1.0 / 2.0 * float3x3(-1.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0, 0.0, -1.0),
	1.0 / 6.0 * float3x3(1.0, -2.0, 1.0, -2.0, 4.0, -2.0, 1.0, -2.0, 1.0),
	1.0 / 6.0 * float3x3(-2.0, 1.0, -2.0, 1.0, 4.0, 1.0, -2.0, 1.0, -2.0),
	1.0 / 3.0 * float3x3(1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0)
};

float4 Edge_FreiChen(PixelInputType input) : SV_TARGET{
	int i, j;
	float3x3 I;
	float3 color;
	float cnv[9];

	[unroll]
	for (i = 0; i < 3; i++) {
		[unroll]
		for (j = 0; j < 3; j++) {
			color = CurrentScene.Sample(TexSampler, (input.texCoord * screenDimentions + float2(i - 1, j - 1)) * texelSize).rgb;
			I[i][j] = length(color);
		}
	}

	[unroll]
	for (i = 0; i < 9; i++) {
		float dp3 = dot(G2[i][0], I[0]) + dot(G2[i][1], I[1]) + dot(G2[i][2], I[2]);
		cnv[i] = dp3 * dp3;
	}

	float M = (cnv[4] + cnv[5]) + (cnv[6] + cnv[7]);
	float S = (cnv[0] + cnv[1]) + (cnv[2] + cnv[3]) + (cnv[4] + cnv[5]) + (cnv[6] + cnv[7]) + cnv[8];
	float v = sqrt(M / S);
	return CurrentScene.Sample(TexSampler, input.texCoord) - float4(v, v, v, 0.0f) * 2.0f;
}

// http://www.geeks3d.com/20110408/cross-stitching-post-processing-shader-glsl-filter-geexlab-pixel-bender/
static const float stitchSize = 6.0f;
static const bool invert = true;

float4 crossStitch(PixelInputType input) : SV_TARGET {
	float4 c = float4(0.0f, 0.0f, 0.0f, 0.0f);
	float2 cPos = input.texCoord * screenDimentions;
	float2 tlPos = floor(cPos / float2(stitchSize, stitchSize));
	tlPos *= stitchSize;
	int remX = cPos.x % stitchSize;
	int remY = cPos.y % stitchSize;
	if (remX == remY) tlPos = cPos;
	float2 blPos = tlPos;
	blPos.y += (stitchSize - 1.0f);
	if (remX == remY || (int)cPos.x - (int)blPos.x == (int)blPos.y - (int)cPos.y) {
		if (invert) {
			return float4(0.2f, 0.15f, 0.05f, 1.0f);
		}
		else {
			return CurrentScene.Sample(TexSampler, tlPos * texelSize) * 1.4f;
		}
	}
	else {
		if (invert) {
			return CurrentScene.Sample(TexSampler, tlPos * texelSize) * 1.4f;
		}
		else {
			return float4(0.0f, 0.0f, 0.0f, 1.0f);
		}
	}
}

// Define our technique here
//   The bloom needs to blur the screen first (we blur it twice, once horizontally, another vertically), then applies the effect
technique11 Bloom
{
	pass horizontalgaussianblur
	{
		VertexShader = compile vs_5_0 mainvs();
		PixelShader = compile ps_5_0 horblurps();
	}
	pass verticalgaussianblur
	{
		PixelShader = compile ps_5_0 verblurps();
	}
	pass bloom
	{
		PixelShader = compile ps_5_0 bloomps();
	}
	pass bloom_p2
	{
		PixelShader = compile ps_5_0 horblurps();
	}
	pass bloom_p3
	{
		PixelShader = compile ps_5_0 verblurps();
	}
}

technique11 DepthOfField
{
	pass dof
	{
		PixelShader = compile ps_5_0 GetDepth();
	}
	pass p1
	{
		PixelShader = compile ps_5_0 DofPS();
	}
	pass p2
	{
		PixelShader = compile ps_5_0 horblurps();
	}
	pass p3
	{
		PixelShader = compile ps_5_0 verblurps();
	}
}

technique11 EdgeDetection
{
	pass Edge
	{
		PixelShader = compile ps_5_0 Edge_FreiChen();
	}
}

/*
technique11 CrossStitch
{
	pass CrossStitch
	{
		PixelShader = compile ps_5_0 crossStitch();
	}
}
*/
