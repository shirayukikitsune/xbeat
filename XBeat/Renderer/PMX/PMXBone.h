#pragma once

#include "PMXDefinitions.h"
#include "../D3DRenderer.h"
#include "GeometricPrimitive.h" // From DirectX Toolkit
#include <list>
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

	__forceinline uint32_t GetId() const { return id; }

	void Initialize(std::shared_ptr<D3DRenderer> d3d);
	void Reset();

	btMatrix3x3 GetLocalAxis();

	Bone* GetParentBone();

	DirectX::XMVECTOR XM_CALLCONV GetPosition();
	DirectX::XMVECTOR XM_CALLCONV GetOffsetPosition();
	DirectX::XMVECTOR XM_CALLCONV GetEndPosition();
	DirectX::XMVECTOR XM_CALLCONV GetRotation();
	__forceinline DirectX::XMVECTOR XM_CALLCONV GetInitialRotation() const { return m_initialRotation; }

	bool Update(bool force = false);

	void Transform(const btVector3& angles, const btVector3& offset, DeformationOrigin origin = DeformationOrigin::User);
	void Rotate(const btVector3& axis, float angle, DeformationOrigin origin = DeformationOrigin::User);
	void Rotate(const btVector3& angles, DeformationOrigin origin = DeformationOrigin::User);
	void Translate(const btVector3& offset, DeformationOrigin origin = DeformationOrigin::User);

	void ApplyMorph(Morph *morph, float weight);

	__forceinline DirectX::XMMATRIX XM_CALLCONV getLocalTransform() const { return m_transform; }

	bool XM_CALLCONV Render(DirectX::FXMMATRIX view, DirectX::CXMMATRIX projection);

	void updateChildren();

	bool wasTouched();

	BT_DECLARE_ALIGNED_ALLOCATOR();

private:
	bool m_dirty;

	Model *model;

	std::list<std::pair<Morph*,float>> appliedMorphs;

	std::vector<Bone*> children;

	DirectX::XMMATRIX m_transform;
	DirectX::XMMATRIX m_inheritTransform;
	DirectX::XMMATRIX m_userTransform;
	DirectX::XMMATRIX m_morphTransform;

	DirectX::XMVECTOR m_initialRotation;

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

	bool m_touched;

#ifdef PMX_TEST
	friend class PMXTest::BoneTest;
#endif
};

}
}
