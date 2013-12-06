#pragma once

#include "PMXDefinitions.h"
#include "../D3DRenderer.h"
#include "GeometricPrimitive.h" // From DirectX Toolkit
#include <list>

namespace Renderer {
namespace PMX {

class Loader;
class Model;

PMX_ALIGN class Bone
{
public:
	Bone(Model *model, uint32_t id);
	~Bone(void);

	friend class Loader;

	__forceinline const Name& GetName() const { return name; }
	__forceinline uint32_t GetParentId() const { return parent; }
	__forceinline int32_t GetDeformationOrder() const { return deformationOrder; }
	__forceinline const Position& GetAxisTranslation() const { return axisTranslation; }

	__forceinline BoneFlags::Flags GetFlags() const { return flags; }
	__forceinline bool HasAllFlags(BoneFlags::Flags flag) const { return (flags & flag) == flag; }
	__forceinline bool HasAnyFlag(BoneFlags::Flags flag) const { return (flags & flag) != 0; }

	void Initialize(std::shared_ptr<D3DRenderer> d3d);
	void Reset();

	btMatrix3x3 GetLocalAxis();

	Bone* GetParentBone();

	Position GetPosition();
	Position GetOffsetPosition();
	Position GetEndPosition();

	void Update();

	void Transform(const btVector3& angles, const btVector3& offset, DeformationOrigin::Id origin = DeformationOrigin::User);
	void Rotate(const btVector3& angles, DeformationOrigin::Id origin = DeformationOrigin::User);
	void Translate(const btVector3& offset, DeformationOrigin::Id origin = DeformationOrigin::User);

	void ApplyMorph(Morph *morph, float weight);

	__forceinline const btTransform& getLocalTransform() const { return m_transform; }

	bool Render(std::shared_ptr<D3DRenderer> d3d, DirectX::CXMMATRIX world, DirectX::CXMMATRIX view, DirectX::CXMMATRIX projection);

	PMX_ALIGNMENT_OPERATORS

private:
	float getVertexWeight(Vertex *vertex);


	void updateChildren();
	void updateVertices();

	Model *model;
	std::list<std::pair<Vertex*, int>> vertices;

	std::list<std::pair<Morph*,float>> appliedMorphs;

	std::list<Bone*> children;

	btTransform m_transform;
	btTransform m_inheritTransform;
	btTransform m_toOriginTransform;
	btTransform m_userTransform;
	btTransform m_morphTransform;

	uint32_t id;

	Name name;
	Position startPosition;
	uint32_t parent;
	int32_t deformationOrder;
	BoneFlags::Flags flags;
	union {
		btScalar length[3];
		uint32_t attachTo;
	} size;
	struct {
		uint32_t from;
		float rate;
	} inherit;
	Position axisTranslation;
	struct {
		Position xDirection;
		Position zDirection;
	} localAxes;
	int externalDeformationKey;
	IK ik;

	// Used for debug render
	std::unique_ptr<DirectX::GeometricPrimitive> m_primitive;

#ifdef PMX_TEST
	friend class PMXTest::BoneTest;
#endif
};

}
}
