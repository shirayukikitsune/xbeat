#pragma once

#include "PMXDefinitions.h"
#include "../D3DRenderer.h"
#include "GeometricPrimitive.h" // From DirectX Toolkit
#include <list>
#include <deque>
#include <vector>

namespace Renderer {
namespace PMX {

class Loader;
class Model;

ATTRIBUTE_ALIGNED16(class) Bone
{
public:
	Bone(Model *model, uint32_t id);
	~Bone(void);

	friend class Loader;

	__forceinline const Name& GetName() const { return name; }
	__forceinline uint32_t GetParentId() const { return parent; }
	__forceinline int32_t GetDeformationOrder() const { return deformationOrder; }
	__forceinline const Position& GetAxisTranslation() const { return axisTranslation; }

	__forceinline BoneFlags GetFlags() const { return (BoneFlags)flags; }
	__forceinline bool HasAllFlags(BoneFlags flag) const { return (flags & (uint16_t)flag) == (uint16_t)flag; }
	__forceinline bool HasAnyFlag(BoneFlags flag) const { return (flags & (uint16_t)flag) != 0; }

	void Initialize(std::shared_ptr<D3DRenderer> d3d);
	void Reset();

	btMatrix3x3 GetLocalAxis();

	Bone* GetParentBone();

	Position GetPosition();
	Position GetOffsetPosition();
	Position GetEndPosition();
	btQuaternion GetRotation();

	bool Update(bool force = false);

	void Transform(const btVector3& angles, const btVector3& offset, DeformationOrigin origin = DeformationOrigin::User);
	void Rotate(const btVector3& angles, DeformationOrigin origin = DeformationOrigin::User);
	void Translate(const btVector3& offset, DeformationOrigin origin = DeformationOrigin::User);

	void ApplyMorph(Morph *morph, float weight);

	__forceinline const btTransform& getLocalTransform() const { return m_transform; }

	bool Render(DirectX::CXMMATRIX world, DirectX::CXMMATRIX view, DirectX::CXMMATRIX projection);

	void updateVertices();
	void updateChildren();

	BT_DECLARE_ALIGNED_ALLOCATOR();

private:
	float getVertexWeight(Vertex *vertex);
	btQuaternion slerp(btScalar fT, const btQuaternion &rkP, const btQuaternion &rkQ, bool shortestPath = true);

	bool m_dirty;

	Model *model;
	std::vector<std::pair<Vertex*, int>> vertices;

	std::list<std::pair<Morph*,float>> appliedMorphs;

	std::deque<Bone*> children;

	btTransform m_transform;
	btTransform m_inheritTransform;
	btTransform m_toOriginTransform;
	btTransform m_userTransform;
	btTransform m_morphTransform;

	btQuaternion m_initialRotation;

	uint32_t id;

	Name name;
	Position startPosition;
	uint32_t parent;
	int32_t deformationOrder;
	uint16_t flags;
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
