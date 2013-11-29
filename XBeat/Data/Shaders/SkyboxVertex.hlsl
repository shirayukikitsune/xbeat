cbuffer MatrixBuffer
{
    matrix wvp;
};

struct SkyboxVsInput
{
	float3 position : SV_POSITION;
	float3 normal : NORMAL0;
	float2 texCoord : TEXCOORD0;
};

struct SkyboxVsOutput	//output structure for skymap vertex shader
{
	float4 Pos : SV_POSITION;
	float3 texCoord : TEXCOORD;
};

SkyboxVsOutput main(SkyboxVsInput input)
{
	SkyboxVsOutput output;

	//Set Pos to xyww instead of xyzw, so that z will always be 1 (furthest from camera)
    output.Pos = mul(float4(input.position, 1.0f), wvp).xyww;

	output.texCoord = input.position;

	return output;
}
