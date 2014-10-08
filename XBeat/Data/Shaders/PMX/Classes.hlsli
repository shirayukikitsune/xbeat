#define MAX_BONES 1000
#define MAX_LIGHTS 3

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

struct BoneBufferType
{
	float4x4 transform;
	float4 position;
};

struct VertexInput
{
	float4 position : SV_Position;
	float3 normal : NORMAL;
	float2 tex : TEXCOORD0;
	uint4 boneIndices : BONES0;
	float4 boneWeights : BONES1;
	uint material : MATERIAL0;
};

struct VertexOutput
{
	float4 position : SV_POSITION;
	float2 tex : TEXCOORD0;
	float3 normal : NORMAL;
	float3 viewDirection : TEXCOORD1;
	float depth : TEXCOORD2;
	uint material : MATERIAL0;
};

typedef VertexInput GeometryIO;
typedef VertexOutput PixelInput;

struct PixelOutput
{
	float4 color : SV_Target;
	//float depth : SV_Depth;
};