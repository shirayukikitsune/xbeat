////////////////////////////////////////////////////////////////////////////////
// Filename: light.vs
////////////////////////////////////////////////////////////////////////////////


/////////////
// GLOBALS //
/////////////
cbuffer MatrixBuffer
{
	matrix worldMatrix;
	matrix wvp;
};

cbuffer CameraBuffer
{
	float3 cameraPosition;
	float padding;
};


//////////////
// TYPEDEFS //
//////////////
struct VertexInputType
{
    float4 position : SV_Position;
	float3 normal : NORMAL;
	float2 tex : TEXCOORD0;
/*	float4 exUV1 : TEXCOORD1;
	float4 exUV2 : TEXCOORD2;
	float4 exUV3 : TEXCOORD3;
	float4 exUV4 : TEXCOORD4;*/
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
    output.position = mul(input.position, wvp);
    
	// Store the texture coordinates for the pixel shader.
	output.tex = input.tex;
    
	// Calculate the normal vector against the world matrix only.
	output.normal = mul(input.normal, (float3x3)worldMatrix);
	
	// Normalize the normal vector.
	output.normal = normalize(output.normal);

	// the view direction is the vector starting at the camera and points towards the vector
	// A->B = B - A
	worldPos = mul(input.position, worldMatrix);
	output.viewDirection = normalize(worldPos.xyz - cameraPosition.xyz);
	
	output.depth = output.position.z / output.position.w;
	
    return output;
}