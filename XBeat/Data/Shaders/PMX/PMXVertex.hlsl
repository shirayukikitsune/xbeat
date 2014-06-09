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

VertexOutput main(VertexInput input)
{
	VertexOutput output;
	float4 worldPos;

	// Change the position vector to be 4 units for proper matrix calculations.
	input.position.w = 1.0f;

	// Calculate the position of the vertex against the world, view, and projection matrices.
	output.position = mul(input.position, matrices.wvp);

	// Store the texture coordinates for the pixel shader.
	output.tex = input.tex;
	output.exUV1 = input.exUV1;
	output.exUV2 = input.exUV2;
	output.exUV3 = input.exUV3;
	output.exUV4 = input.exUV4;
	output.material = input.material;

	// Calculate the normal vector against the world matrix only.
	output.normal = normalize(mul(input.normal, (float3x3)matrices.world));

	// the view direction is the vector starting at the camera and points towards the vector
	// A->B = B - A
	worldPos = mul(input.position, matrices.world);
	output.viewDirection = normalize(worldPos.xyz - eyePosition.xyz);

	output.depth = output.position.z / output.position.w;

	return output;
}