#pragma once

#include <string>
#include <map>
#include <vector>
#include <cstdint>

#include <DirectXMath.h>

#include "../Model.h"

namespace Renderer {
namespace PMX {
struct Header {
	char abMagic[4];
	float fVersion;
};

struct SizeInfo {
	uint8_t cbSize;
	uint8_t cbEncoding;
	uint8_t cbUVVectorSize;
	uint8_t cbVertexIndexSize;
	uint8_t cbTextureIndexSize;
	uint8_t cbMaterialIndexSize;
	uint8_t cbBoneIndexSize;
	uint8_t cbMorphIndexSize;
	uint8_t cbRigidBodyIndexSize;
};

struct vec3f {
	float x, y, z;
};
typedef vec3f Position;
struct vec4f {
	float x, y, z, w;
};

struct RenderMaterial;

struct Vertex {
	Position position;
	Position normal;
	float uv[2];
	std::vector<float> uvEx;
	uint8_t weightMethod;
	union {
		struct {
			uint32_t boneIndexes[4];
			float weights[4];
		} BDEF;
		struct {
			uint32_t boneIndexes[2];
			float weightBias;
			Position C;
			Position R0;
			Position R1;
		} SDEF;
	} boneInfo;
	float edgeWeight;

	RenderMaterial *material;
	Position offset;
};

struct Color {
	float red;
	float green;
	float blue;
};

struct Color4 : public Color {
	float alpha;
};

struct MaterialFlags {
	enum : uint8_t {
		DoubleSide = 0x01,
		GroundShadow = 0x02,
		DrawSelfShadowMap = 0x04,
		DrawSelfShadow = 0x08,
		DrawEdge = 0x10
	};
};

struct MaterialSphereMode {
	enum : uint8_t {
		Disabled,
		Multiply,
		Add
	};
};

struct Material {
	std::wstring nameJP, nameEN;
	Color4 diffuse;
	Color specular;
	float specularCoefficient;
	Color ambient;
	uint8_t flags;
	Color4 edgeColor;
	float edgeSize;
	uint32_t baseTexture;
	uint32_t sphereTexture;
	uint8_t sphereMode;
	uint8_t toonFlag;
	union {
		uint32_t custom;
		uint8_t default;
	} toonTexture;
	std::wstring freeField;
	int indexCount;
};

struct BoneFlags {
	enum : uint16_t {
		Attached = 0x0001,
		Rotatable = 0x0002,
		Movable = 0x0004,
		View = 0x0008,
		Manipulable = 0x0010,
		IK = 0x0020,
		InheritDeformation = 0x0080,
		InheritRotation = 0x0100,
		InheritTranslation = 0x0200,
		TranslateAxis = 0x0400,
		LocalAxis = 0x0800,
		Flagx1000 = 0x1000, // need good translation for 「物理後変形」
		Flagx2000 = 0x2000 // need good translation for 「外部親変形」
	};
};

struct IKnode {
	uint32_t bone;
	bool limitAngle;
	struct {
		Position lower;
		Position upper;
	} limits;
};

struct IK {
	uint32_t target;
	int count;
	float angleLimit;
	std::vector<IKnode> links;
};

struct Bone {
	std::wstring nameJP, nameEN;
	Position position;
	uint32_t parent;
	int32_t deformation;
	uint16_t flags;
	union {
		Position length;
		uint32_t attachTo;
	} size;
	struct {
		uint32_t index;
		float rate;
	} rotationInherit;
	Position axisTranslation;
	struct {
		Position xDirection;
		Position zDirection;
	} localAxes;
	int externalDeformationKey;
	IK ik;
};

struct MaterialMorph {
	uint32_t index;
	uint8_t method;
	Color4 diffuse;
	Color specular;
	float specularCoefficient;
	Color ambient;
	Color4 edgeColor;
	float edgeSize;
	Color4 baseCoefficient;
	Color4 sphereCoefficient;
	Color4 toonCoefficient;
};

union MorphType {
	struct {
		uint32_t index;
		Position offset;
	} vertex;
	struct {
		uint32_t index;
		vec4f offset;
	} uv;
	struct {
		uint32_t index;
		Position movement;
		vec4f rotation;
	} bone;
	MaterialMorph material;
	struct {
		uint32_t index;
		float rate;
	} group;

	// Just for organization, it is not used
	enum MorphTypeId {
		Group,
		Vertex,
		Bone,
		UV,
		UV1,
		UV2,
		UV3,
		UV4,
		Material
	};
};

struct Morph {
	std::wstring nameJP, nameEN;
	uint8_t operation;
	uint8_t type;
	std::vector<MorphType> data;
};

struct FrameMorphs {
	uint8_t target;
	uint32_t id;
};

struct Frame {
	std::wstring nameJP, nameEN;
	uint8_t type;
	std::vector<FrameMorphs> morphs;
};

struct RigidBody {
	std::wstring nameJP, nameEN;
	uint32_t targetBone;
	uint8_t group;
	uint16_t nonCollisionFlag;
	uint8_t shape;
	vec3f size;
	vec3f position;
	vec3f rotation;
	float mass;
	float inertia;
	float rotationDampening;
	float repulsion;
	float friction;
	uint8_t mode;
};

struct SpringJoint {
	uint32_t bodyA, bodyB;
	vec3f position;
	vec3f rotation;
	vec3f lowerMovementRestrictions;
	vec3f upperMovementRestrictions;
	vec3f lowerRotationRestrictions;
	vec3f upperRotationRestrictions;
	vec3f movementSpringConstant;
	vec3f rotationSpringConstant;
};

struct Joint {
	std::wstring nameJP, nameEN;
	uint8_t type;
	union {
		SpringJoint spring;
	};
};

struct RenderMaterial
{
	RenderMaterial() {
		baseTexture.reset();
		sphereTexture.reset();
		toonTexture.reset();

		vertexBuffer = nullptr;
		indexBuffer = nullptr;
		indexCount = 0;
		materialIndex = 0;
		startIndex = 0;
	}
	~RenderMaterial() {
		Shutdown();
	}

	void Shutdown() {
		baseTexture.reset();
		sphereTexture.reset();
		toonTexture.reset();

		if (vertexBuffer != nullptr) {
			vertexBuffer->Release();
			vertexBuffer = nullptr;
		}
		if (indexBuffer != nullptr) {
			indexBuffer->Release();
			indexBuffer = nullptr;
		}
	}

	DirectX::XMFLOAT4 getDiffuse(Material *m);
	DirectX::XMFLOAT4 getAmbient(Material *m);
	DirectX::XMFLOAT4 getAverage(Material *m);
	DirectX::XMFLOAT4 getSpecular(Material *m);
	float getSpecularCoefficient(Material *m);

	std::shared_ptr<Texture> baseTexture;
	std::shared_ptr<Texture> sphereTexture;
	std::shared_ptr<Texture> toonTexture;

	uint32_t materialIndex, startIndex;
	ID3D11Buffer *vertexBuffer;
	ID3D11Buffer *indexBuffer;
	int indexCount;
	DirectX::XMFLOAT4 avgColor;
	DirectX::XMFLOAT3 center, radius;

	MaterialMorph additive, multiplicative;
};

class Model : public Renderer::Model
{
public:
	Model(void);
	virtual ~Model(void);

	struct {
		std::wstring nameJP, commentJP;
		std::wstring nameEN, commentEN;
	} description;

	Position GetBonePosition(const std::wstring &JPname);
	Position GetBoneEndPosition(const std::wstring &JPname);

	void ApplyMorph(const std::wstring &JPname, float weight);

private:
	Header *header;
	SizeInfo *sizeInfo;

	std::vector<Vertex*> vertices;
	std::vector<uint32_t> verticesIndex;
	std::vector<std::wstring> textures;
	std::vector<Material*> materials;
	std::vector<Bone*> bones;
	std::vector<Morph*> morphs;
	std::vector<Frame*> frames;
	std::vector<RigidBody*> bodies;
	std::vector<Joint*> joints;

	std::vector<RenderMaterial> rendermaterials;

	uint64_t lastpos;

	std::vector<std::shared_ptr<Texture>> renderTextures;

	std::wstring basePath;

	static std::vector<std::shared_ptr<Texture>> sharedToonTextures;

protected:
	virtual bool InitializeBuffers(ID3D11Device *device);
	virtual void ShutdownBuffers();
	virtual bool RenderBuffers(std::shared_ptr<D3DRenderer> d3d, std::shared_ptr<Shaders::Light> lightShader, DirectX::CXMMATRIX view, DirectX::CXMMATRIX projection, DirectX::CXMMATRIX world, std::shared_ptr<Light> light, std::shared_ptr<CameraClass> camera, std::shared_ptr<ViewFrustum> frustum);

	virtual bool LoadModel(const std::wstring &filename);
	virtual void ReleaseModel();

	virtual bool LoadTexture(ID3D11Device *device);
	virtual void ReleaseTexture();

private:
	void applyVertexMorph(Morph* morph, float weight);
	void applyMaterialMorph(Morph* morph, float weight);
	void applyMaterialMorph(MorphType* morph, RenderMaterial* material, float weight);
	void updateMaterialVertexBuffer(RenderMaterial* material);

	void generateAdditiveMaterialMorph(MaterialMorph &morph);
	void generateMultiplicativeMaterialMorph(MaterialMorph &morph);

	bool loadModel(const std::wstring &filename);
	std::wstring getString(std::istream &is);
};

}
}
