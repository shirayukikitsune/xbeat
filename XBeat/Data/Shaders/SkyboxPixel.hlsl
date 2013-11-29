TextureCube SkyMap;
SamplerState SampleType;

cbuffer MatrixBuffer
{
    matrix worldMatrix;
    matrix viewMatrix;
    matrix projectionMatrix;
};

struct SkyboxVsOutput	//output structure for skymap vertex shader
{
	float4 Pos : SV_POSITION;
	float3 texCoord : TEXCOORD;
};

float4 main(SkyboxVsOutput input) : SV_Target
{
	//return float4(0.7f, 0.7f, 0.7f, 1.0f);
	return SkyMap.Sample(SampleType, input.texCoord);
}

