/////////////
// GLOBALS //
/////////////
struct MatrixBuffer
{
	matrix world;
	matrix view;
	matrix projection;
	matrix wvp;
	matrix worldInverse;
};
struct LightBuffer
{
	float4 ambient;
	float4 diffuse;
	float3 specularColor;
	float specularPower;
	float4 direction;
	float4 position;
};

cbuffer data : register (cb0)
{
	MatrixBuffer matrices;
	LightBuffer lights[3];

	float3 eyePosition;
	float ___padding;
	uint1 lightCount;
};


//////////////
// TYPEDEFS //
//////////////
struct VertexInputType
{
    float4 position : SV_Position;
	float3 normal : NORMAL;
	float2 tex : TEXCOORD0;
};

struct PixelInputType
{
	float4 position : SV_POSITION;
	float2 tex : TEXCOORD0;
	float3 normal : NORMAL;
	float3 viewDirection : TEXCOORD1;
	float depth : TEXCOORD2;
};


////////////////////////////////////////////////////////////////////////////////
// Vertex Shader
////////////////////////////////////////////////////////////////////////////////
PixelInputType main(VertexInputType input)
{
    PixelInputType output;
	float4 worldPos;

	// Change the position vector to be 4 units for proper matrix calculations.
    input.position.w = 1.0f;
	 
	// Calculate the position of the vertex against the world, view, and projection matrices.
	output.position = mul(input.position, matrices.wvp);

	// Store the texture coordinates for the pixel shader.
	output.tex = input.tex;

	// Calculate the normal vector against the world matrix only.
	output.normal = mul(input.normal, (float3x3)matrices.world);
	
	// Normalize the normal vector.
	output.normal = normalize(output.normal);

	// the view direction is the vector starting at the camera and points towards the vertex
	// A->B = B - A
	worldPos = mul(input.position, matrices.world);
	output.viewDirection = normalize(worldPos.xyz - eyePosition.xyz);
	
	output.depth = output.position.z / output.position.w;
	
    return output;
}