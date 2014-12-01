#include "PMXBone.h"

#include "PMXMaterial.h"
#include "PMXModel.h"

#include <algorithm>
#include <cfloat>

namespace PMX {
namespace detail {
	class RootBone
		: public Bone
	{
	public:
		RootBone(PMX::Model *Model) : Bone(Model, -1) {
			Flags = (uint16_t)BoneFlags::Manipulable | (uint16_t)BoneFlags::Movable | (uint16_t)BoneFlags::Rotatable;
		}

		virtual bool isRootBone() { return true; }

		virtual Bone* getRootBone() { return this; }

		virtual btVector3 getStartPosition() { return btVector3(0, 0, 0); }

		virtual void initialize(Loader::Bone *Data) {};
		virtual void terminate() {};
		virtual void update() {};

		virtual void transform(const btVector3& Angles, const btVector3& Offset, DeformationOrigin Origin = DeformationOrigin::User)
		{
			btQuaternion Rotation;
			Rotation.setEulerZYX(Angles.x(), Angles.y(), Angles.z());
			Transform.setRotation(Rotation * Transform.getRotation());
			Transform.setOrigin(Transform.getOrigin() + Offset);

			Inverse = Transform.inverse();
		}

		virtual void transform(const btTransform& Transform, DeformationOrigin Origin = DeformationOrigin::User)
		{
			this->Transform *= Transform;

			Inverse = this->Transform.inverse();
		}

		virtual void rotate(const btVector3& Axis, float Angle, DeformationOrigin Origin = DeformationOrigin::User)
		{
			rotate(btQuaternion(Axis, Angle), Origin);
		}

		virtual void rotate(const btVector3& Angles, DeformationOrigin Origin = DeformationOrigin::User)
		{
			btQuaternion Rotation;
			Rotation.setEulerZYX(Angles.x(), Angles.y(), Angles.z());
			rotate(Rotation, Origin);
		}

		virtual void rotate(const btQuaternion& Rotation, DeformationOrigin Origin = DeformationOrigin::User)
		{
			Transform.setRotation(Rotation * Transform.getRotation());

			Inverse = Transform.inverse();
		}

		virtual void translate(const btVector3& Offset, DeformationOrigin Origin = DeformationOrigin::User)
		{
			Transform.setOrigin(Transform.getOrigin() + Offset);

			Inverse = Transform.inverse();
		}

		virtual void resetTransform()
		{
			Transform.setIdentity();
			Inverse.setIdentity();
		}

#if defined _M_IX86 && defined _MSC_VER
		void *__cdecl operator new(size_t count){
			return _aligned_malloc(count, 16);
		}

			void __cdecl operator delete(void *object) {
			_aligned_free(object);
		}
#endif
	};

	class BoneImpl
		: public Bone
	{
	public:
		BoneImpl(PMX::Model *Model, uint32_t Id) : Bone(Model, Id){};

		virtual btVector3 getPosition();

		virtual void initialize(Loader::Bone *Data);
		virtual void initializeDebug(ID3D11DeviceContext *Context);
		virtual void terminate();

		virtual void update();

		virtual void transform(const btVector3& Angles, const btVector3& Offset, DeformationOrigin Origin = DeformationOrigin::User);
		virtual void transform(const btTransform& Transform, DeformationOrigin Origin = DeformationOrigin::User);
		virtual void rotate(const btVector3& Axis, float Angle, DeformationOrigin Origin = DeformationOrigin::User);
		virtual void rotate(const btVector3& Angles, DeformationOrigin Origin = DeformationOrigin::User);
		virtual void rotate(const btQuaternion& Rotation, DeformationOrigin Origin = DeformationOrigin::User);
		virtual void translate(const btVector3& Offset, DeformationOrigin Origin = DeformationOrigin::User);

		virtual void resetTransform();

		virtual void applyMorph(Morph *morph, float weight);
		virtual void applyPhysicsTransform(btTransform &transform);

		virtual void XM_CALLCONV render(DirectX::FXMMATRIX world, DirectX::CXMMATRIX view, DirectX::CXMMATRIX projection);

		virtual btVector3 getStartPosition();

		virtual Bone* getRootBone();

		virtual void clearIK() { IkRotation = btQuaternion::getIdentity(); }

		/**
		 * @returns the length of this bone
		 */
		float getLength();
		btVector3 getEndPosition(bool transform = true);
		btQuaternion IkRotation;

#if defined _M_IX86 && defined _MSC_VER
		void *__cdecl operator new(size_t count){
			return _aligned_malloc(count, 16);
		}

			void __cdecl operator delete(void *object) {
			_aligned_free(object);
		}
#endif

	protected:
		btVector3 getOffsetPosition();

		std::list<std::pair<Morph*, float>> appliedMorphs;

		btTransform InheritTransform, UserTransform, MorphTransform;

		btMatrix3x3 LocalAxis;
		btVector3 AxisTranslation;
		btVector3 InitialPosition, EndPosition;

		btQuaternion DebugRotation;

		BoneImpl *AttachedTo;
		BoneImpl *InheritFrom;
		float InheritRate;

		float Length;

		// Used for debug render
		std::unique_ptr<DirectX::GeometricPrimitive> Primitive;
	};

	class IKBone
		: public BoneImpl
	{
	public:
		IKBone(PMX::Model *Model, uint32_t Id) : BoneImpl(Model, Id) {}

		virtual void initialize(Loader::Bone *Data);
		virtual void initializeDebug(ID3D11DeviceContext *Context);
		virtual void terminate();

		virtual bool isIK() { return true; }
		virtual void performIK();

#if defined _M_IX86 && defined _MSC_VER
		void *__cdecl operator new(size_t count){
			return _aligned_malloc(count, 16);
		}

		void __cdecl operator delete(void *object) {
			_aligned_free(object);
		}
#endif

	private:
		struct Node {
			BoneImpl* Bone;
			bool Limited;
			struct {
				float Lower[3];
				float Upper[3];
			} Limits;
		};

		BoneImpl *TargetBone;

		float ChainLength;

		int LoopCount;

		float AngleLimit;

		std::vector<Node> Links;
	};
}
}

using namespace PMX;

Bone* Bone::createBone(PMX::Model *Model, uint32_t Id, BoneType Type)
{
	switch (Type) {
	case BoneType::Root:
		return new detail::RootBone(Model);
	case BoneType::Regular:
		return new detail::BoneImpl(Model, Id);
	case BoneType::IK:
		return new detail::IKBone(Model, Id);
	}

	return nullptr;
}

void Bone::updateChildren() {
	for (auto &Child : m_children) {
		Child->update();
		Child->updateChildren();
	}
}

void detail::BoneImpl::initialize(Loader::Bone *Data)
{
	InheritTransform = UserTransform = MorphTransform = btTransform::getIdentity();
	IkRotation = btQuaternion::getIdentity();

	InitialPosition = DirectX::XMFloat3ToBtVector3(Data->InitialPosition);

	Inverse.setOrigin(-getStartPosition());
	Transform.setOrigin(getStartPosition());

	Parent = Model->GetBoneById(getParentId());
	Parent->m_children.push_back(this);

	using namespace DirectX;
	if (!hasAnyFlag((uint16_t)BoneFlags::LocalAxis)) {
		LocalAxis.setIdentity();

		auto Parent = dynamic_cast<detail::BoneImpl*>(this->Parent);
		if (Parent) LocalAxis = Parent->LocalAxis;
	}
	else {
		LocalAxis[0] = DirectX::XMFloat3ToBtVector3(Data->LocalAxes.X).normalized();
		LocalAxis[2] = DirectX::XMFloat3ToBtVector3(Data->LocalAxes.Z).normalized();
		LocalAxis[1] = LocalAxis[0].cross(LocalAxis[2]).normalized();
		LocalAxis[2] = LocalAxis[0].cross(LocalAxis[1]).normalized();
		AxisTranslation = DirectX::XMFloat3ToBtVector3(Data->AxisTranslation);
	}

	if (!hasAnyFlag((uint16_t)BoneFlags::Attached)) {
		Length = sqrtf(Data->Size.Length[0] * Data->Size.Length[0] + Data->Size.Length[1] * Data->Size.Length[1] + Data->Size.Length[2] * Data->Size.Length[2]);
		EndPosition = btVector3(Data->Size.Length[0], Data->Size.Length[1], Data->Size.Length[2]);
	}
	else {
		AttachedTo = dynamic_cast<BoneImpl*>(Model->GetBoneById(Data->Size.AttachTo));
		if (AttachedTo) Length = AttachedTo->getStartPosition().distance(getStartPosition());
		else Length = 0.0f;
	}

	btVector3 up, axis, direction;
	up = btVector3(0, 1, 0);
	direction = getEndPosition() - getPosition();
	axis = up.cross(direction);
	if (axis.length2() > 0) {
		// cosO = a.b/||a||*||b||
		DebugRotation.setRotation(axis, acosf(up.dot(direction) / direction.length()));
	}
	else DebugRotation = btQuaternion::getIdentity();
}

void detail::BoneImpl::initializeDebug(ID3D11DeviceContext *Context)
{
	if (hasAnyFlag((uint16_t)BoneFlags::View))
		Primitive = DirectX::GeometricPrimitive::CreateCylinder(Context);
}

void detail::BoneImpl::terminate()
{
	Primitive.reset();
}

btVector3 detail::BoneImpl::getOffsetPosition()
{
	return getStartPosition() - Parent->getStartPosition();
}

btVector3 detail::BoneImpl::getEndPosition(bool transform)
{
	btVector3 p;

	if (hasAnyFlag((uint16_t)BoneFlags::Attached)) {
		p = transform ? AttachedTo->getPosition() : AttachedTo->getStartPosition();
	}
	else {
		p = btVector3();
		if (transform) p = quatRotate(Transform.getRotation(), p) + getPosition();
	}

	return p;
}

float detail::BoneImpl::getLength()
{
	return Length;
}

btVector3 detail::BoneImpl::getPosition()
{
	return Transform.getOrigin();
}

btVector3 detail::BoneImpl::getStartPosition()
{
	return InitialPosition;
}

void detail::BoneImpl::update()
{
	btVector3 Translation(0, 0, 0);
	btQuaternion Rotation = btQuaternion::getIdentity();

	if (hasAnyFlag((uint16_t)BoneFlags::LocallyAttached)) {
		Translation = Parent->getLocalTransform().getOrigin();
	}
	else if (hasAnyFlag((uint16_t)BoneFlags::TranslationAttached)) {
		if (InheritFrom) Translation = InheritFrom->InheritTransform.getOrigin();
	}
	Translation *= InheritRate;

	if (hasAnyFlag((uint16_t)BoneFlags::LocallyAttached)) {
		Rotation = Parent->getLocalTransform().getRotation();
	}
	else {
		if (hasAnyFlag((uint16_t)BoneFlags::RotationAttached)) {
			if (InheritFrom) {
				Rotation = InheritFrom->InheritTransform.getRotation();
			}
		}
	}
	Rotation = btQuaternion::getIdentity().slerp(Rotation, InheritRate);

	InheritTransform = btTransform(Rotation, Translation);

	Translation = (Translation + UserTransform.getOrigin() + MorphTransform.getOrigin() + getOffsetPosition());

	Rotation *= UserTransform.getRotation();
	Rotation *= MorphTransform.getRotation();
	Rotation *= IkRotation;
	Rotation.normalize();

	Transform = hasAnyFlag((uint16_t)BoneFlags::LocallyAttached) ? btTransform(Rotation, Translation) : Parent->getTransform() * btTransform(Rotation, Translation);
}

void detail::BoneImpl::transform(const btVector3& angles, const btVector3& offset, DeformationOrigin origin)
{
	if (origin == DeformationOrigin::User && !hasAllFlags((uint16_t)BoneFlags::Manipulable | (uint16_t)BoneFlags::Movable))
		return;

	btQuaternion local;
	local.setEulerZYX(angles.x(), angles.y(), angles.z());

	transform(btTransform(local, offset), origin);
}

void detail::BoneImpl::transform(const btTransform& transform, DeformationOrigin origin)
{
	if (origin == DeformationOrigin::User && !hasAllFlags((uint16_t)BoneFlags::Manipulable | (uint16_t)BoneFlags::Movable))
		return;

	if (origin == DeformationOrigin::Motion) {
		UserTransform = transform;
		return;
	}

	UserTransform *= transform;
}

void detail::BoneImpl::rotate(const btVector3& axis, float angle, DeformationOrigin origin)
{
	if (origin == DeformationOrigin::User && !hasAllFlags((uint16_t)BoneFlags::Manipulable | (uint16_t)BoneFlags::Rotatable))
		return;

	btQuaternion local;
	local.setRotation(axis, angle);

	rotate(local, origin);
}

void detail::BoneImpl::rotate(const btVector3& angles, DeformationOrigin origin)
{
	if (origin == DeformationOrigin::User && !hasAllFlags((uint16_t)BoneFlags::Manipulable | (uint16_t)BoneFlags::Rotatable))
		return;

	btQuaternion local;
	local.setEulerZYX(angles.x(), angles.y(), angles.z());

	rotate(local, origin);
}

void detail::BoneImpl::rotate(const btQuaternion& Rotation, DeformationOrigin Origin)
{
	if (Origin == DeformationOrigin::User && !hasAllFlags((uint16_t)BoneFlags::Manipulable | (uint16_t)BoneFlags::Rotatable))
		return;

	UserTransform.setRotation(Rotation * UserTransform.getRotation());
}

void detail::BoneImpl::translate(const btVector3& offset, DeformationOrigin origin)
{
	if (origin == DeformationOrigin::User && !hasAllFlags((uint16_t)BoneFlags::Manipulable | (uint16_t)BoneFlags::Movable))
		return;

	UserTransform.setOrigin(UserTransform.getOrigin() + offset);
}

void detail::BoneImpl::resetTransform()
{
	UserTransform.setIdentity();
}

void detail::BoneImpl::applyPhysicsTransform(btTransform &transform)
{
	Transform = transform;
}

void detail::BoneImpl::applyMorph(Morph *morph, float weight)
{
	auto i = ([this, morph](){ for (auto i = appliedMorphs.begin(); i != appliedMorphs.end(); ++i) { if (i->first == morph) return i; } return appliedMorphs.end(); })();
	if (weight == 0.0f) {
		if (i != appliedMorphs.end())
			appliedMorphs.erase(i);
		else return;
	}
	else {
		if (i == appliedMorphs.end())
			appliedMorphs.push_back(std::make_pair(morph, weight));
		else
			i->second = weight;
	}

	// Now calculate the transformations
	btVector3 position(0, 0, 0);
	btQuaternion rotation = btQuaternion::getIdentity();
	for (auto &pair : appliedMorphs) {
		for (auto &data : pair.first->data) {
			if (data.bone.index == getId()) {
				position += pair.second * btVector3(data.bone.movement[0], data.bone.movement[1], data.bone.movement[2]);
				rotation *= btQuaternion::getIdentity().slerp(btQuaternion(data.bone.rotation[0], data.bone.rotation[1], data.bone.rotation[2], data.bone.rotation[3]), pair.second);
				rotation.normalize();
			}
		}
	}
	MorphTransform.setRotation(rotation);
	MorphTransform.setOrigin(position);
}

void XM_CALLCONV detail::BoneImpl::render(DirectX::FXMMATRIX world, DirectX::CXMMATRIX view, DirectX::CXMMATRIX projection)
{
	DirectX::XMMATRIX w;

	if (Primitive)
	{
		w = DirectX::XMMatrixAffineTransformation(DirectX::XMVectorSet(0.3f, 0.3f, 0.3f, 1), DirectX::XMVectorZero(), getRotation().get128(), getPosition().get128());

		Primitive->Draw(w, view, projection, DirectX::Colors::Red);
	}
}

Bone* detail::BoneImpl::getRootBone() { 
	return Model->GetRootBone();
}

void detail::IKBone::initialize(Loader::Bone *Data) {
	BoneImpl::initialize(Data);

	TargetBone = dynamic_cast<detail::BoneImpl*>(Model->GetBoneById(Data->IkData->targetIndex));
	assert(TargetBone != nullptr);
	ChainLength = 0.0f;
	AngleLimit = Data->IkData->angleLimit;
	LoopCount = Data->IkData->loopCount;

	for (auto &Link : Data->IkData->links) {
		Node NewNode;
		NewNode.Bone = dynamic_cast<detail::BoneImpl*>(Model->GetBoneById(Link.boneIndex));
		assert(NewNode.Bone != nullptr);

		NewNode.Limited = Link.limitAngle;
		std::copy(&NewNode.Limits.Lower[0], &NewNode.Limits.Upper[3], &Link.limits.lower[0]);

		// Hack to fix models imported from PMD
		if (NewNode.Bone->getName().japanese.find(L"ひざ") != std::wstring::npos && NewNode.Limited == false) {
			NewNode.Limited = true;
			NewNode.Limits.Lower[0] = -DirectX::XM_PI;
			NewNode.Limits.Lower[1] = NewNode.Limits.Lower[2] = NewNode.Limits.Upper[0] = NewNode.Limits.Upper[1] = NewNode.Limits.Upper[2] = 0.0f;
		}
		ChainLength += NewNode.Bone->getLength();
		Links.emplace_back(NewNode);
	}

	assert(!Links.empty());
	assert(ChainLength > 0.00001f);
}

void detail::IKBone::initializeDebug(ID3D11DeviceContext *Context)
{
	Primitive = DirectX::GeometricPrimitive::CreateSphere(Context, 2.5f);
}

void detail::IKBone::terminate()
{
	Primitive.reset();
}

#if 1
void detail::IKBone::performIK() {
	if (TargetBone->isSimulated())
		return;

	btVector3 TargetPosition = getPosition();
	btVector3 RootPosition = Links.back().Bone->getPosition();

	// Determine if the target position is reachable
	if (RootPosition.distance(TargetPosition) > ChainLength) {
		// If unreachable, move the target point to a reachable point colinear to the root bone position and the original target point
		TargetPosition = (TargetPosition - RootPosition).normalized() * ChainLength + RootPosition;
		assert(RootPosition.distance(TargetPosition) <= ChainLength + 0.0001f);
	}

	btQuaternion OldRotation = TargetBone->getRotation();

	for (int Iteration = 0; Iteration < LoopCount; ++Iteration) {
		btVector3 EndPosition = TargetBone->getPosition();
		// Check if the last joint position is close to the target point
		if (EndPosition.distance(TargetPosition) < 0.001f) {
			return;
		}

		// Here we are assuming that all bones of the chain are interconnected
		std::vector<btVector3> TargetPositions;
		// The first position of the vector is the root of the chain, and the last, the target point
		for (int Index = Links.size() - 1; Index >= 0; --Index) {
			TargetPositions.emplace_back(Links[Index].Bone->getPosition());
		}
		
		TargetPositions.emplace_back(TargetPosition);

		auto performInnerIteraction = [](btVector3 &ParentBonePosition, btVector3 &LinkPosition, btVector3 &TargetPosition, Node &Link, bool Reverse) {
			float Distance = LinkPosition.distance(TargetPosition);
			if (Distance <= 0.00001f) {
				LinkPosition = TargetPosition;
				return;
			}
			float BoneLength = Link.Bone->getLength();
			float Ratio = BoneLength / Distance;
			btClamp(Ratio, 0.0f, 1.0f);

			LinkPosition = TargetPosition.lerp(LinkPosition, Ratio);

#if 1
			// Now apply rotation constraints if needed
			if (!Link.Limited || ParentBonePosition.isZero())
				return;

			btVector3 LineDirection = (TargetPosition - ParentBonePosition).normalize();
			btVector3 Origin = LineDirection.dot(LinkPosition - TargetPosition) * LineDirection;
			float DistanceToOrigin = Origin.length();
			Origin += TargetPosition;

			btVector3 LocalXDirection(1, 0, 0);
			btQuaternion YRotation;
			{
				// Calculate the Y axis rotation and rotate the X axis for that rotation
				float YRotationAngle = btAcos(LineDirection.dot(btVector3(0, 1, 0)));
				btVector3 YRotationAxis = LineDirection.cross(btVector3(0, 1, 0));
				YRotation = btQuaternion(YRotationAxis, YRotationAngle);
				LocalXDirection = quatRotate(YRotation, LocalXDirection);
			}

			btVector3 LocalPosition = LinkPosition - Origin;
			float Theta = btAcos(LocalPosition.normalized().dot(LocalXDirection));
			int Quadrant = (int)(Theta / DirectX::XM_PIDIV2);
			float Q1, Q2;

			switch (Quadrant) {
			default:
				Q1 = DistanceToOrigin * tanf(Link.Limits.Upper[0]);
				Q2 = DistanceToOrigin * tanf(Link.Limits.Upper[2]);
				break;
			case 1:
				Q1 = DistanceToOrigin * tanf(Link.Limits.Lower[0]);
				Q2 = DistanceToOrigin * tanf(Link.Limits.Upper[2]);
				break;
			case 2:
				Q1 = DistanceToOrigin * tanf(Link.Limits.Lower[0]);
				Q2 = DistanceToOrigin * tanf(Link.Limits.Lower[2]);
				break;
			case 3:
				Q1 = DistanceToOrigin * tanf(Link.Limits.Upper[0]);
				Q2 = DistanceToOrigin * tanf(Link.Limits.Lower[2]);
				break;
			}

			float X = Q1 * cosf(Theta);
			float Y = Q2 * sinf(Theta);
			LocalPosition.setX(powf(-1.0f, Quadrant) * (fabsf(X) < fabsf(LocalPosition.x()) ? fabsf(X) : fabsf(LocalPosition.x())));
			LocalPosition.setZ(powf(-1.0f, Quadrant) * (fabsf(Y) < fabsf(LocalPosition.z()) ? fabsf(Y) : fabsf(LocalPosition.z())));

			LinkPosition = (quatRotate(YRotation.inverse(), LocalPosition) + Origin).lerp(LinkPosition, BoneLength / DistanceToOrigin);
#endif
		};

		// Stage 1: Forward reaching
		btVector3 P0(0, 0, 0);
		for (int Index = TargetPositions.size() - 2; Index >= 0; --Index) {
			auto& Link = Links[Links.size() - Index - 1];
			performInnerIteraction(P0, TargetPositions[Index], TargetPositions[Index + 1], Link, false);
			P0 = TargetPositions[Index];
		}

		// Set the root position its initial position
		TargetPositions[0] = RootPosition;

		// Stage 2: Backward reaching
		P0.setZero();
		for (int Index = 0; Index < TargetPositions.size() - 1; ++Index) {
			auto& Link = Links[Links.size() - Index - 1];
			performInnerIteraction(P0, TargetPositions[Index + 1], TargetPositions[Index], Link, true);
			P0 = TargetPositions[Index];
		}

		// Apply the new positions as rotations
		for (int Index = TargetPositions.size() - 2; Index >= 0; --Index) {
			auto& Link = Links[Links.size() - Index - 1];
			btVector3 position = Link.Bone->getPosition();

			btVector3 CurrentDirection = (Link.Bone->getEndPosition(true) - position).normalized();
			btVector3 DesiredDirection = (TargetPositions[Index + 1] - position).normalized();

			btVector3 Axis = CurrentDirection.cross(DesiredDirection);
			float Dot = CurrentDirection.dot(DesiredDirection);
			// No need to update if position is too close
			if (Axis.length2() < 0.000001f || Dot > 0.999999f) continue;
			
			Axis.normalize();
			float Angle = btAcos(Dot);

			Link.Bone->IkRotation *= btQuaternion(Axis, Angle);

			Link.Bone->update();

			position = Link.Bone->getPosition();
		}
		TargetBone->update();
	}

	TargetBone->IkRotation = OldRotation * TargetBone->getRotation().inverse();
	TargetBone->update();
}
#else
void detail::BoneImpl::PerformIK() {
	btVector3 Destination = this->GetPosition();
	btVector3 RootPosition = ikData->links.back().bone->GetPosition();

	auto InitialRotation = ikData->targetBone->GetTransform().getRotation();

#if 1
	// Determine if the target position is reachable
	if (RootPosition.distance(Destination) > ikData->chainLength) {
		// If unreachable, move the target point to a reachable point colinear to the root bone position and the original target point
		Destination = (Destination - RootPosition).normalized() * ikData->chainLength + RootPosition;
		assert(RootPosition.distance(Destination) <= ikData->chainLength + 0.0001f);
	}
#endif

	for (int Iteration = 0; Iteration < ikData->loopCount; ++Iteration) {
		for (int Index = 0; Index < ikData->links.size(); ++Index) {
			auto &Link = ikData->links[Index];
			auto Bone = static_cast<detail::BoneImpl*>(Link.bone);
			btVector3 CurrentPosition = Bone->GetPosition();
			btVector3 AffectedBonePosition = ikData->targetBone->GetPosition();

			if (CurrentPosition == Destination || CurrentPosition == AffectedBonePosition) continue;

			btTransform TransformInverse = Link.bone->GetTransform().inverse();
			btVector3 LocalDestination = TransformInverse(Destination);
			btVector3 LocalAffectedBonePosition = TransformInverse(AffectedBonePosition);

			if (LocalDestination.distance2(LocalAffectedBonePosition) <= 0.0001f) {
				Iteration = ikData->loopCount;
				break;
			}

			LocalDestination.normalize();
			LocalAffectedBonePosition.normalize();

			auto Dot = LocalDestination.dot(LocalAffectedBonePosition);
			if (Dot > 1.0f) continue;

			auto Angle = acosf(Dot);
			if (fabsf(Angle) < 0.0001f) continue;

			btClamp(Angle, -ikData->angleLimit, ikData->angleLimit);

			btVector3 Axis = LocalAffectedBonePosition.cross(LocalDestination);
			if (Axis.length2() < 0.0001f && Iteration != 0) continue;

			Axis.normalize();

			btQuaternion Rotation(Axis, Angle);
			// clamp rotation values
			if (Link.limitAngle) {
#if 0
				if (Iteration == 0) {
					if (Angle < 0.0f)
						Angle = -Angle;
					Bone->m_ikRotation.setRotation(btVector3(1.0f, 0.0f, 0.0f), Angle);
				}
				else
#endif
				{
					btMatrix3x3 Matrix;
					float x, y, z;
					Matrix.setRotation(Rotation);
					Matrix.getEulerZYX(z, y, x);
					float cx, cy, cz;
					Matrix.setRotation(Bone->getRotation());
					Matrix.getEulerZYX(cz, cy, cx);

#if 0
					x += cx;
					y += cy;
					z += cz;
					btClamp(x, Link.limits.lower[0], Link.limits.upper[0]);
					btClamp(y, Link.limits.lower[1], Link.limits.upper[1]);
					btClamp(z, Link.limits.lower[2], Link.limits.upper[2]);
#else
					if (x + cx > Link.limits.upper[0])
						x = Link.limits.upper[0] - cx;
					if (x + cx < Link.limits.lower[0])
						x = Link.limits.lower[0] - cx;

					if (y + cy > Link.limits.upper[1])
						y = Link.limits.upper[1] - cy;
					if (y + cy < Link.limits.lower[1])
						y = Link.limits.lower[1] - cy;

					if (z + cz > Link.limits.upper[2])
						z = Link.limits.upper[2] - cz;
					if (z + cz < Link.limits.lower[2])
						z = Link.limits.lower[2] - cz;
#endif
					//Rotation.setEulerZYX(z, y, x);

					Bone->m_ikRotation.setEulerZYX(z, y, x);
				}
			}
			else 
				Bone->m_ikRotation *= Rotation;

			for (int UpdateIndex = Index; UpdateIndex >= 0; --UpdateIndex) {
				ikData->links[UpdateIndex].bone->Update();
			}
			ikData->targetBone->Update();
		}
	}

	ikData->targetBone->m_ikRotation = InitialRotation * ikData->targetBone->getRotation().inverse();
	ikData->targetBone->Update();
}
#endif
