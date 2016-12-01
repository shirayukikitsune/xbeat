#pragma once

#include <Urho3D/Urho3D.h>
#include <Urho3D/Graphics/Skeleton.h>
#include <Urho3D/Math/Vector3.h>
#include <Urho3D/Scene/Component.h>

class PMXIKNode
	: public Urho3D::Component
{
	URHO3D_OBJECT(PMXIKNode, Urho3D::Component);

public:
	PMXIKNode(Urho3D::Context *context);
	~PMXIKNode();
	/// Register object factory.
	static void RegisterObject(Urho3D::Context* context);

	void SetLimited(bool value) { limited = value; }
	void SetBoneLength(float value) { boneLength = value; }
	void SetLowerLimit(const Urho3D::Vector3& value) { lowerLimit = value; }
	void SetUpperLimit(const Urho3D::Vector3& value) { upperLimit = value; }
	void SetBone(Urho3D::Bone *bone) { associatedBone = bone; }

	bool IsLimited() const { return limited; }
	float GetBoneLength() const { return boneLength; }
	const Urho3D::Vector3& GetLowerLimit() const { return lowerLimit; }
	const Urho3D::Vector3& GetUpperLimit() const { return upperLimit; }
	Urho3D::Bone* GetBone() const { return associatedBone; }

private:
	bool limited;
	float boneLength;
	Urho3D::Bone *associatedBone;
	Urho3D::Vector3 lowerLimit;
	Urho3D::Vector3 upperLimit;
};

