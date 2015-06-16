#pragma once

#include "PMXDefinitions.h"
#include "PMXLoader.h"
#include "../Renderer/D3DRenderer.h"

#include <vector>

namespace PMX {

class Model;
class RigidBody;

class Bone
{
public:
	static Bone* createBone(Model *Model, uint32_t Id, BoneType Type);

	//! Applies a rotation around Euler angles and a translation
	virtual void transform(const btVector3& Angles, const btVector3& Offset, DeformationOrigin Origin = DeformationOrigin::User) = 0;
	//! Applies a btTransform
	virtual void transform(const btTransform& Transform, DeformationOrigin Origin = DeformationOrigin::User) = 0;
	//! Rotates around an axis
	virtual void rotate(const btVector3& Axis, float Angle, DeformationOrigin Origin = DeformationOrigin::User) = 0;
	//! Rotates using euler angles
	virtual void rotate(const btVector3& Angles, DeformationOrigin Origin = DeformationOrigin::User) = 0;
	//! Rotates using a quaternion
	virtual void rotate(const btQuaternion& Rotation, DeformationOrigin Origin = DeformationOrigin::User) = 0;
	//! Apply a translation
	virtual void translate(const btVector3& Offset, DeformationOrigin Origin = DeformationOrigin::User) = 0;
	//! Resets the bone to the original position
	virtual void resetTransform() = 0;

	//! Initialize the bone
	virtual void initialize(Loader::Bone *Data) = 0;
	//! Initialize the debug renderer
	virtual void initializeDebug(ID3D11DeviceContext *Context) {}
	//! Prepare for destruction
	virtual void terminate() = 0;

	//! Debug render
	virtual void XM_CALLCONV render(DirectX::FXMMATRIX World, DirectX::CXMMATRIX View, DirectX::CXMMATRIX Projection) {}

	//! Returns the parent bone ID
	uint32_t getParentId() { return ParentId; }
	//! Returns the parent bone
	Bone* getParent() { return Parent; }
	//! Return this bone ID
	uint32_t getId() { return Id; }
	//! Return the parent model
	Model* getModel() { return Model; }

	//! Returns the name of this bone
	const Name& getName() const { return Name; }

	//! Returns whether this is the root bone or not
	virtual bool isRootBone() { return false; }
	//! Returns the root bone
	virtual Bone* getRootBone() { return nullptr; }
	//! Returns whether this is an IK bone
	virtual bool isIK() { return false; }

	//! Gets the global transformation
	btTransform getTransform() { return Transform; }
	//! Gets the local transformation
	btTransform getLocalTransform() { return Inverse * Transform; }
	//! Gets the transformation used for skinning
	virtual btTransform getSkinningTransform() { return Inverse * Transform; }
	//! Gets the inverse transformation
	btTransform getInverseTransform() { return Inverse; }

	//! Returns the position of the bone
	virtual btVector3 getPosition() { return getTransform().getOrigin(); }
	//! Returns the initial position of the bone
	virtual btVector3 getStartPosition() = 0;
	//! Returns the rotation of this bone
	btQuaternion getRotation() { return getTransform().getRotation(); }
	//! Returns the IK component of the rotation of this bone
	virtual btQuaternion getIKRotation() { return btQuaternion::getIdentity(); }

	virtual void applyMorph(Morph *Morph, float Weight) {}

	virtual void applyPhysicsTransform(btTransform &Transform) {}

	//! Update the transformation each frame
	virtual void update() = 0;

	//! Perform IK link
	virtual void performIK() {}
	//! Clear IK information
	virtual void clearIK() {}

	//! Returns the flags for this bone
	BoneFlags getFlags() const { return (BoneFlags)Flags; }
	//! Check if this bone has all of the specified flags
	bool hasAllFlags(uint16_t Flag) const { return (Flags & Flag) == Flag; }
	//! Check if this bone has any of the specified flags
	bool hasAnyFlag(uint16_t Flag) const { return (Flags & Flag) != 0; }

	//! Returns the physical deformation order
	int32_t getDeformationOrder() const { return DeformationOrder; }

	std::vector<Bone*> m_children;
	void updateChildren();

	void setSimulated(bool State = true) { Simulated = State; }
	bool isSimulated() { return Simulated; }

	btTransform Transform;

private:
	const uint32_t Id;

protected:
	Bone(Model *Model, uint32_t Id)
		: Id(Id), Model(Model) {
		Parent = nullptr;
		ParentId = -1;
		Flags = 0;
		DeformationOrder = 0;
		Transform.setIdentity();
		Inverse.setIdentity();
		Simulated = false;
	}

	//! Disallow copy and move constructors
	Bone(const Bone &other) = delete;
	Bone(Bone &&other) = delete;

	Name Name;
	Model *Model;
	Bone *Parent;
	uint16_t Flags;
	int32_t DeformationOrder;
	uint32_t ParentId;

	btTransform Inverse;

	bool Simulated;
};
}
