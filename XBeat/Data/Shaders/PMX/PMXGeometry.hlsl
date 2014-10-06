#include "Classes.hlsli"

StructuredBuffer<BoneBufferType> bones : register(t0);

[maxvertexcount(3)]
void main(
	triangle GeometryIO input[3] : SV_POSITION,
	inout TriangleStream< GeometryIO > output
)
{
	[unroll]
	for (uint i = 0; i < 3; i++)
	{
		GeometryIO element;
		element.position = 0;
		element.position.w = 1.0f;
		element.tex = input[i].tex;
		element.normal = input[i].normal;
		element.boneIndices = input[i].boneIndices;
		element.boneWeights = input[i].boneWeights;
		element.material = input[i].material;

		float4x4 skinning = 0;

		[unroll]
		for (uint j = 0; j < 4; j++) {
			skinning += bones[element.boneIndices[j]].transform * element.boneWeights[j];
			element.position += element.boneWeights[j] * mul(input[i].position, bones[element.boneIndices[j]].transform);
			//element.position += (mul(input[i].position - bones[element.boneIndices[j]].position, bones[element.boneIndices[j]].transform) + bones[element.boneIndices[j]].position) * element.boneWeights[j];
 		}
		//element.position = mul(element.position, skinning);
		element.normal = normalize(mul(element.normal, (float3x3)skinning));

		output.Append(element);
	}
}