#pragma once

#include <string>
#include <map>
#include <vector>
#include <cstdint>
#include <array>

#include <DirectXMath.h>

#include "../Renderer/Model.h"
#include "../Physics/Environment.h"

#include "PMXDefinitions.h"
#include "PMXLoader.h"
#include "PMXSoftBody.h"
#include "PMXRigidBody.h"
#include "PMXJoint.h"
#include "PMXShader.h"
#include "PMXBone.h"

namespace PMX {

class Model : public Renderer::Model
{
public:
	Model(void);
	virtual ~Model(void);

	ModelDescription description;

	DirectX::XMVECTOR GetBonePosition(const std::wstring &JPname);
	DirectX::XMVECTOR GetBoneEndPosition(const std::wstring &JPname);
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
	std::shared_ptr<RigidBody> GetRigidBodyById(uint32_t id);
	std::shared_ptr<RigidBody> GetRigidBodyByName(const std::wstring &JPname);

	virtual bool Update(float msec);
	virtual void Render(ID3D11DeviceContext *context, std::shared_ptr<Renderer::ViewFrustum> frustum);

	std::shared_ptr<Renderer::D3DRenderer> m_d3d;

	virtual bool LoadModel(const std::wstring &filename);

private:
	std::vector<Vertex*> vertices;

	std::vector<uint32_t> verticesIndex;
	std::vector<std::wstring> textures;
	std::vector<Material*> materials;
	std::vector<Bone*> bones;
	std::vector<Morph*> morphs;
	std::vector<Frame*> frames;
	std::vector<Loader::RigidBody*> bodies;
	std::vector<Loader::Joint*> joints;
	std::vector<SoftBody*> softBodies;

	Bone *rootBone;

	std::vector<RenderMaterial> rendermaterials;

	uint64_t lastpos;

	std::vector<std::shared_ptr<Renderer::Texture>> renderTextures;

	std::wstring basePath;

	static std::vector<std::shared_ptr<Renderer::Texture>> sharedToonTextures;

	std::vector<std::shared_ptr<RigidBody>> m_rigidBodies;
	std::vector<std::shared_ptr<Joint>> m_joints;

	std::vector<PMXShader::VertexType> m_vertices;

	bool updateVertexBuffer(ID3D11DeviceContext *Context);
	bool updateMaterialBuffer(uint32_t material, ID3D11DeviceContext *context);
	bool m_dirtyBuffer;
	ID3D11Buffer *m_materialBuffer;
	ID3D11Buffer *m_vertexBuffer, *m_tmpVertexBuffer;
	ID3D11Buffer *m_indexBuffer;

	uint32_t m_debugFlags;

	std::vector<Bone*> m_prePhysicsBones;
	std::vector<Bone*> m_postPhysicsBones;
	std::vector<detail::BoneImpl*> m_ikBones;

	friend detail::RootBone;

protected:
	virtual bool InitializeBuffers(std::shared_ptr<Renderer::D3DRenderer> d3d);
	virtual void ShutdownBuffers();

	virtual void ReleaseModel();

	virtual bool LoadTexture(ID3D11Device *device);
	virtual void ReleaseTexture();

private:
	void applyVertexMorph(Morph* morph, float weight);
	void applyMaterialMorph(Morph* morph, float weight);
	void applyMaterialMorph(MorphType* morph, RenderMaterial* material, float weight);
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
