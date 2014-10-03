#pragma once

#include "PMXDefinitions.h"
#include "../Renderer/D3DRenderer.h"
#include "GeometricPrimitive.h" // From DirectX Toolkit
#include <list>

namespace PMX {

class Loader;
class Model;
class RigidBody;

class Bone
{
public:
	//! Applies a rotation around Euler angles and a translation
	virtual void Transform(DirectX::FXMVECTOR angles, DirectX::FXMVECTOR offset, DeformationOrigin origin = DeformationOrigin::User) = 0;
	//! Applies a btTransform
	virtual void Transform(const DirectX::XMTRANSFORM& transform, DeformationOrigin origin = DeformationOrigin::User) = 0;
	//! Rotates around an axis
	virtual void Rotate(DirectX::FXMVECTOR axis, float angle, DeformationOrigin origin = DeformationOrigin::User) = 0;
	//! Rotates using euler angles
	virtual void Rotate(DirectX::FXMVECTOR angles, DeformationOrigin origin = DeformationOrigin::User) = 0;
	//! Apply a translation
	virtual void Translate(DirectX::FXMVECTOR offset, DeformationOrigin origin = DeformationOrigin::User) = 0;
	//! Resets the bone to the original position
	virtual void ResetTransform() = 0;

	//! Initialize the bone
	virtual void Initialize() = 0;
	//! Prepare for destruction
	virtual void Terminate() = 0;

	//! Returns the parent bone ID
	uint32_t GetParentId() { return m_parentId; }
	//! Returns the parent bone
	Bone* GetParent() { return m_parent; }
	//! Return this bone ID
	uint32_t GetId() { return m_id; }
	//! Return the parent model
	Model* GetModel() { return m_model; }

	//! Returns whether this is the root bone or not
	virtual bool IsRootBone() { return false; }
	//! Returns the root bone
	virtual Bone* GetRootBone() { return nullptr; }

	//! Gets the global transformation
	virtual DirectX::XMTRANSFORM GetTransform() = 0;
	//! Gets the local transformation
	virtual DirectX::XMTRANSFORM GetLocalTransform() = 0;
	//! Gets the inverse transformation
	DirectX::XMTRANSFORM GetInverseTransform() { return m_inverse; }
	//! Gets the direct transformation
	DirectX::XMTRANSFORM GetDirectTransform() { return m_transform; }

	//! Returns the position of the bone
	virtual DirectX::XMVECTOR GetPosition() { return GetTransform().GetTranslation(); }
	//! Returns the initial position of the bone
	virtual DirectX::XMVECTOR XM_CALLCONV GetStartPosition() = 0;

	virtual void ApplyPhysicsTransform(DirectX::XMTRANSFORM &transform) {}

	//! Update the transformation each frame
	virtual void Update() = 0;

	//! Returns the flags for this bone
	BoneFlags GetFlags() const { return (BoneFlags)m_flags; }
	//! Check if this bone has all of the specified flags
	bool HasAllFlags(BoneFlags flag) const { return (m_flags & (uint16_t)flag) == (uint16_t)flag; }
	//! Check if this bone has any of the specified flags
	bool HasAnyFlag(BoneFlags flag) const { return (m_flags & (uint16_t)flag) != 0; }

	//! Returns the physical deformation order
	int32_t GetDeformationOrder() const { return m_deformationOrder; }

	std::vector<Bone*> m_children;

private:
	const uint32_t m_id;

protected:
	Bone(Model *model, uint32_t id)
		: m_id(id) {
		m_model = model;
		m_parent = nullptr;
		m_parentId = -1;
		m_flags = 0;
		m_deformationOrder = 0;
	}

	//! Disallow copy and move constructors
	Bone(const Bone &other) = delete;
	Bone(Bone &&other) = delete;

	Model *m_model;
	Bone *m_parent;
	uint16_t m_flags;
	int32_t m_deformationOrder;
	uint32_t m_parentId;

	DirectX::XMTRANSFORM m_transform, m_inverse;
	DirectX::XMVECTOR m_position;
};

namespace detail {
class RootBone
	: public Bone
{
public:
	RootBone(Model *model) : Bone(model, -1) {
		m_flags = (uint16_t)BoneFlags::Manipulable | (uint16_t)BoneFlags::Movable | (uint16_t)BoneFlags::Rotatable;
		m_touched = false;
	}

	virtual bool IsRootBone() { return true; }
	// We do not override the GetRootBone, or else GetTransform would enter an infinite loop

	//! Gets the global transformation
	virtual DirectX::XMTRANSFORM GetTransform() { return m_transform; }
	//! Gets the local transformation
	virtual DirectX::XMTRANSFORM GetLocalTransform() { return m_transform; }

	virtual DirectX::XMVECTOR XM_CALLCONV GetStartPosition() { return DirectX::XMVectorZero(); }

	// Nothing to do with these
	virtual void Initialize() {};
	virtual void Terminate() {};

	virtual void Transform(DirectX::FXMVECTOR angles, DirectX::FXMVECTOR offset, DeformationOrigin origin = DeformationOrigin::User);
	virtual void Transform(const DirectX::XMTRANSFORM& transform, DeformationOrigin origin = DeformationOrigin::User);
	virtual void Rotate(DirectX::FXMVECTOR axis, float angle, DeformationOrigin origin = DeformationOrigin::User);
	virtual void Rotate(DirectX::FXMVECTOR angles, DeformationOrigin origin = DeformationOrigin::User);
	virtual void Translate(DirectX::FXMVECTOR offset, DeformationOrigin origin = DeformationOrigin::User);
	virtual void ResetTransform();

	virtual void Update();

private:
	bool m_touched;
};

class BoneImpl
	: public Bone
{
public:
	BoneImpl(Model *model, uint32_t id) : Bone(model, id){};
	~BoneImpl(void);

	friend class Loader;

	virtual DirectX::XMVECTOR GetPosition();

	const Name& GetName() const { return name; }
	const DirectX::XMVECTOR& GetAxisTranslation() const { return axisTranslation; }

	virtual void Initialize();
	void InitializeDebug(std::shared_ptr<Renderer::D3DRenderer> d3d);
	virtual void Terminate();

	DirectX::XMMATRIX XM_CALLCONV GetLocalAxis();

	virtual void Update();

	virtual void Transform(DirectX::FXMVECTOR angles, DirectX::FXMVECTOR offset, DeformationOrigin origin = DeformationOrigin::User);
	virtual void Transform(const DirectX::XMTRANSFORM& transform, DeformationOrigin origin = DeformationOrigin::User);
	virtual void Rotate(DirectX::FXMVECTOR axis, float angle, DeformationOrigin origin = DeformationOrigin::User);
	virtual void Rotate(DirectX::FXMVECTOR angles, DeformationOrigin origin = DeformationOrigin::User);
	virtual void Translate(DirectX::FXMVECTOR offset, DeformationOrigin origin = DeformationOrigin::User);

	virtual void ResetTransform();
	
	void ApplyMorph(Morph *morph, float weight);
	virtual void ApplyPhysicsTransform(DirectX::XMTRANSFORM &transform);

	bool XM_CALLCONV Render(DirectX::FXMMATRIX world, DirectX::CXMMATRIX view, DirectX::CXMMATRIX projection);

	DirectX::XMVECTOR XM_CALLCONV GetOffsetPosition();
	DirectX::XMVECTOR XM_CALLCONV GetEndPosition();
	virtual DirectX::XMVECTOR XM_CALLCONV GetStartPosition();

	//! Gets the global transformation
	virtual DirectX::XMTRANSFORM GetTransform();
	//! Gets the local transformation
	virtual DirectX::XMTRANSFORM GetLocalTransform();

	virtual Bone* GetRootBone();

private:
	bool m_dirty;

	void setDirty();

	std::list<std::pair<Morph*,float>> appliedMorphs;

	DirectX::XMVECTOR m_inheritRotation, m_inheritTranslation;
	DirectX::XMVECTOR m_userRotation, m_userTranslation;
	DirectX::XMVECTOR m_morphRotation, m_morphTranslation;
	DirectX::XMMATRIX m_localAxis;
	DirectX::XMVECTOR m_debugRotation;

	Name name;
	DirectX::XMVECTOR startPosition;
	union {
		float length[3];
		uint32_t attachTo;
	} size;
	struct {
		uint32_t from;
		float rate;
	} inherit;
	DirectX::XMVECTOR axisTranslation;
	struct {
		DirectX::XMVECTOR xDirection;
		DirectX::XMVECTOR zDirection;
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
