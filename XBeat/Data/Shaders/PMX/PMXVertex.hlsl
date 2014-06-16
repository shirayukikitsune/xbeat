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

StructuredBuffer<BoneBufferType> bones : register(t0);

VertexOutput main(VertexInput input)
{
	VertexOutput output;
	float4 worldPos;

	// Change the position vector to be 4 units for proper matrix calculations.
	input.position.w = 1.0f;

	// Transform the vertex position for each bone that affects it
	matrix finalTransform = 0;
	[unroll]
	for (uint i = 0; i < 4; i++) {
		if (input.boneWeights[i] <= 0.0f) continue;
		finalTransform += bones[input.boneIndices[i]].transform * input.boneWeights[i];
	}
	if (determinant(finalTransform) != 0) {
		output.position = mul(input.position, finalTransform);
	}
	else output.position = input.position;

	// Calculate the position of the vertex against the world, view, and projection matrices.
	output.position = mul(output.position, matrices.wvp);

	// Store the texture coordinates for the pixel shader.
	output.tex = input.tex;
	/*output.exUV1 = input.exUV1;
	output.exUV2 = input.exUV2;
	output.exUV3 = input.exUV3;
	output.exUV4 = input.exUV4;*/
	output.material = input.material;

	// Calculate the normal vector against the world matrix only.
	output.normal = normalize(mul(mul(input.normal, (float3x3)finalTransform), (float3x3)matrices.worldInverse));

	// the view direction is the vector starting at the camera and points towards the vector
	// A->B = B - A
	worldPos = mul(input.position, matrices.world);
	output.viewDirection = normalize(eyePosition.xyz - worldPos.xyz);

	output.depth = output.position.z / output.position.w;

	return output;
}
