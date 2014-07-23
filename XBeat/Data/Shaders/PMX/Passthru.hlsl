#include "Classes.hlsli"

StructuredBuffer<BoneBufferType> bones : register(t0);

GeometryIO main(VertexInput input)
{
	GeometryIO output;

	output.position = float4(input.position.xyz, 1.0f);
	output.tex = input.tex;
	output.material = input.material;
	output.normal = input.normal;
	output.boneIndices = input.boneIndices;
	output.boneWeights = input.boneWeights;

	return output;
}