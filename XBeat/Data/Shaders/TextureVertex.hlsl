struct VertexInputType
{
    float4 position : POSITION;
    float2 tex : TEXCOORD0;
};

struct PixelInputType
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
};

PixelInputType main(VertexInputType input)
{
    PixelInputType output;

    // Change the position vector to be 4 units for proper matrix calculations.
    input.position.w = 1.0f;

    // Calculate the position of the vertex against the world, view, and projection matrices.
	output.position = float4(sign(input.position.x), sign(input.position.y), 0.0f, 1.0f);

    // Store the texture coordinates for the pixel shader.
    output.tex = input.tex;
    
    return output;
}

