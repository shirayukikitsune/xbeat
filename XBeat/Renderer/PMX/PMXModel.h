#pragma once

#include <string>
#include <map>
#include <vector>
#include <cstdint>
#include <array>

#include <DirectXMath.h>

#include "../Model.h"
#include "../../Physics/Environment.h"

#include "PMXDefinitions.h"
#include "PMXLoader.h"
#include "PMXSoftBody.h"
#include "PMXRigidBody.h"

namespace Renderer {
namespace PMX {

class Model : public Renderer::Model
{
public:
	Model(void);
	virtual ~Model(void);

	struct {
		Name name;
		Name comment;
	} description;

	Position GetBonePosition(const std::wstring &JPname);
	Position GetBoneEndPosition(const std::wstring &JPname);
	Bone* GetBoneByName(const std::wstring &JPname);
	Bone* GetBoneByENName(const std::wstring &ENname);
	Bone* GetBoneById(uint32_t id);
	__forceinline Bone* GetRootBone() { return rootBone; }

	void ApplyMorph(const std::wstring &JPname, float weight);
	void ApplyMorph(Morph *morph, float weight);

	struct DebugFlags {
		enum Flags : uint32_t {
			None,
			RenderBones = 0x1,
			RenderJoints = 0x2,
			RenderSoftBodies = 0x4,
			RenderRigidBodies = 0x8,
			DontRenderModel = 0x10,
			DontUpdatePhysics = 0x20,
		};
	};

	__forceinline DebugFlags::Flags GetDebugFlags() { return (DebugFlags::Flags)m_debugFlags; }
	__forceinline void SetDebugFlags(DebugFlags::Flags value) { m_debugFlags |= value; }
	__forceinline void ToggleDebugFlags(DebugFlags::Flags value) { m_debugFlags ^= value; }
	__forceinline void UnsetDebugFlags(DebugFlags::Flags value) { m_debugFlags &= ~value; }

	Material* GetMaterialById(uint32_t id);
	RenderMaterial* GetRenderMaterialById(uint32_t id);

	virtual void Render(ID3D11DeviceContext *context, std::shared_ptr<ViewFrustum> frustum);

	std::shared_ptr<D3DRenderer> m_d3d;

private:
	std::vector<Vertex*> vertices;
	std::vector<Vertex*> m_sortedVertices;
	std::array<uint32_t, (size_t)VertexWeightMethod::Count> m_vertexCountPerMethod;

	std::vector<uint32_t> verticesIndex;
	std::vector<std::wstring> textures;
	std::vector<Material*> materials;
	std::vector<Bone*> bones;
	std::vector<Morph*> morphs;
	std::vector<Frame*> frames;
	std::vector<Loader::RigidBody*> bodies;
	std::vector<Joint*> joints;
	std::vector<SoftBody*> softBodies;

	Bone *rootBone;

	std::vector<RenderMaterial> rendermaterials;

	uint64_t lastpos;

	std::vector<std::shared_ptr<Texture>> renderTextures;

	std::wstring basePath;

	static std::vector<std::shared_ptr<Texture>> sharedToonTextures;

	std::vector<std::shared_ptr<RigidBody>> m_rigidBodies;

	std::vector<DirectX::VertexPositionNormalTexture> m_vertices;

	bool updateMaterialBuffer(uint32_t material, ID3D11DeviceContext *context);
	bool m_dirtyBuffer;
	ID3D11Buffer *m_materialBuffer;
	ID3D11Buffer *m_vertexBuffer;
	ID3D11Buffer *m_indexBuffer;

	uint32_t m_debugFlags;

protected:
	virtual bool InitializeBuffers(std::shared_ptr<D3DRenderer> d3d);
	virtual void ShutdownBuffers();
	//virtual bool RenderBuffers(std::shared_ptr<D3DRenderer> d3d, std::shared_ptr<Shaders::Light> lightShader, DirectX::CXMMATRIX view, DirectX::CXMMATRIX projection, DirectX::CXMMATRIX world, std::shared_ptr<Light> light, std::shared_ptr<Camera> camera, std::shared_ptr<ViewFrustum> frustum);

	virtual bool LoadModel(const std::wstring &filename);
	virtual void ReleaseModel();

	virtual bool LoadTexture(ID3D11Device *device);
	virtual void ReleaseTexture();

private:
	void applyVertexMorph(Morph* morph, float weight);
	void applyMaterialMorph(Morph* morph, float weight);
	void applyMaterialMorph(MorphType* morph, RenderMaterial* material, float weight);
	void updateVertexBuffer();
	void applyBoneMorph(Morph* morph, float weight);
	void applyFlipMorph(Morph* morph, float weight);
	void applyImpulseMorph(Morph* morph, float weight);

	bool loadModel(const std::wstring &filename);
	bool loadModel(std::istream &data);

	friend class Loader;
#ifdef PMX_TEST
	friend class PMXTest::BoneTest;
#endif
};

}
}
