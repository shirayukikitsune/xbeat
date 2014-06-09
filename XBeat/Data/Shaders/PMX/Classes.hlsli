struct MatrixBuffer
{
	matrix world;
	matrix view;
	matrix projection;
	matrix wvp;
};

struct LightBuffer
{
	float4 ambient;
	float4 diffuse;
	float4 specular;
	float4 direction;
	float4 position;
};

struct MaterialBufferType
{
	float4 ambient;
	float4 diffuse;
	float4 specular;
	float4 mulBaseCoefficient;
	float4 mulSphereCoefficient;
	float4 mulToonCoefficient;
	float4 addBaseCoefficient;
	float4 addSphereCoefficient;
	float4 addToonCoefficient;

	unsigned int flags;
	float weight;
	int index;
	int padding;
};

struct VertexInput
{
	float4 position : SV_Position;
	float3 normal : NORMAL;
	float2 tex : TEXCOORD0;
	/*float4 exUV1 : TEXCOORD1;
	float4 exUV2 : TEXCOORD2;
	float4 exUV3 : TEXCOORD3;
	float4 exUV4 : TEXCOORD4;*/
	uint4 boneIndices : BONES0;
	uint4 boneWeights : BONES1;
	uint material : MATERIAL0;
};

struct VertexOutput
{
	float4 position : SV_POSITION;
	float2 tex : TEXCOORD0;
	float3 normal : NORMAL;
	float3 viewDirection : TEXCOORD1;
	float depth : TEXCOORD2;
	/*float4 exUV1 : TEXCOORD3;
	float4 exUV2 : TEXCOORD4;
	float4 exUV3 : TEXCOORD5;
	float4 exUV4 : TEXCOORD6;*/
	uint material : MATERIAL0;
};

typedef VertexOutput PixelInput;
