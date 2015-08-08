#include "PMXAnimatedModel.h"
#include "PMXIKNode.h"
#include "PMXIKSolver.h"
#include "PMXModel.h"
#include "PMXRigidBody.h"

#include <Urho3D/Core/Context.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Technique.h>
#include <Urho3D/Graphics/Texture2D.h>
#include <Urho3D/Physics/CollisionShape.h>
#include <Urho3D/Physics/Constraint.h>
#include <Urho3D/Physics/RigidBody.h>
#include <Urho3D/Resource/Image.h>
#include <Urho3D/Resource/ResourceCache.h>

using namespace Urho3D;

PMXAnimatedModel::PMXAnimatedModel(Urho3D::Context *context)
	: AnimatedModel(context)
{
}


PMXAnimatedModel::~PMXAnimatedModel()
{
}

void PMXAnimatedModel::RegisterObject(Context* context)
{
	context->RegisterFactory<PMXAnimatedModel>();
}

void PMXAnimatedModel::OnNodeSet(Node* node)
{
	AnimatedModel::OnNodeSet(node);

	if (node)
	{
		// If this AnimatedModel is the first in the node, it is the master which controls animation & morphs
		isMaster_ = GetComponent<PMXAnimatedModel>() == this;
	}
}

void PMXAnimatedModel::SetModel(PMXModel *model)
{
	if (!node_ || !model)
		return;

	auto name = model->GetName();
	model->SetName(name + ".mdl");

	{
		File f(context_, model->GetName(), FILE_WRITE);
		model->Save(f);
		f.Close();
	}

	AnimatedModel::SetModel(model, true);

	auto getBoneLength = [](Vector<PMX::Bone> &boneList, PMX::Bone &bone) {
		Vector3 length = Vector3::ZERO;

		if (bone.Flags & PMX::BoneFlags::Attached) {
			if (bone.Size.AttachTo != -1) {
				PMX::Bone &attached = boneList[bone.Size.AttachTo];
				length = Vector3(attached.InitialPosition) - Vector3(bone.InitialPosition);
			}
		}
		else length = Vector3(bone.Size.Length);

		return length.Length();
	};

	// Define IK chains and calculate offset matrices
	for (unsigned int i = 0; i < model->boneList.Size(); ++i) {
		auto bone = model->boneList[i];
		if ((bone.Flags & PMX::BoneFlags::IK) == 0)
			continue;

		WeakPtr<Node> node;
		node = GetSkeleton().GetBone(i)->node_;

		Matrix3x4 offset = Matrix3x4::IDENTITY;
		int current = i;
		for (Bone* b = GetSkeleton().GetBone(current); b != NULL && b->node_ != NULL && b->node_->GetParent() != GetNode(); b = GetSkeleton().GetBone(current)) {
			offset = offset * Matrix3x4(b->initialPosition_, b->initialRotation_, b->initialScale_);

			current = b->parentIndex_;
		}
		GetSkeleton().GetBone(i)->offsetMatrix_ = offset.Inverse();

		auto ikTarget = node->CreateComponent<PMXIKSolver>();
		ikTarget->SetAngleLimit(bone.IkData.angleLimit);
		ikTarget->SetLoopCount(bone.IkData.loopCount);
		auto endNode = GetSkeleton().GetBone(bone.IkData.targetIndex)->node_;
		ikTarget->AddNode(endNode->CreateComponent<PMXIKNode>());
		ikTarget->SetEndNode(endNode);

		for (auto it = bone.IkData.links.Begin(); it != bone.IkData.links.End(); ++it) {
			auto boneNode = GetSkeleton().GetBone(it->boneIndex)->node_;
			auto ikNode = boneNode->CreateComponent<PMXIKNode>();
			ikNode->SetBoneLength(getBoneLength(model->boneList, model->boneList[it->boneIndex]));
			ikNode->SetLimited(it->limitAngle);
			ikNode->SetLowerLimit(Vector3(it->limits.lower));
			ikNode->SetUpperLimit(Vector3(it->limits.upper));
			ikNode->SetBone(GetSkeleton().GetBone(it->boneIndex));
			ikTarget->AddNode(ikNode);
		}
	}

	Vector<Urho3D::RigidBody*> createdRigidBodies;
	// Create rigid bodies
	for (unsigned int rigidBodyIndex = 0; rigidBodyIndex < model->rigidBodyList.Size(); ++rigidBodyIndex)
	{
		auto Body = model->rigidBodyList[rigidBodyIndex];
		WeakPtr<Node> Node;
		if (Body.targetBone == -1)
			Node = GetSkeleton().GetRootBone()->node_;
		else
			Node = GetSkeleton().GetBone(Body.targetBone)->node_;

		auto Rigid = Node->CreateComponent<Urho3D::RigidBody>();
		createdRigidBodies.Push(Rigid);
		if (Body.mode == PMX::RigidBodyMode::Static) {
			Rigid->SetKinematic(true);
		}
		else {
			Rigid->SetMass(Body.mass);
		}

		if (Body.mode == PMX::RigidBodyMode::AlignedDynamic) {
			Node->CreateComponent<PMXRigidBody>();
		}

		Rigid->SetAngularDamping(Body.angularDamping);
		Rigid->SetFriction(Body.friction);
		Rigid->SetLinearDamping(Body.linearDamping);
		Rigid->SetRestitution(Body.restitution);
		Rigid->SetCollisionLayerAndMask(1 << Body.group, Body.groupMask);
		auto collider = Node->CreateComponent<Urho3D::CollisionShape>();

		Vector3 positionOffset = Vector3(Body.position) - Node->GetWorldPosition() + node_->GetWorldPosition();
		switch (Body.shape) {
		case PMX::RigidBodyShape::Box:
			collider->SetShapeType(SHAPE_BOX);
			collider->SetBox(Vector3(Body.size), positionOffset, Quaternion(Body.rotation[0] * M_RADTODEG, Body.rotation[1] * M_RADTODEG, Body.rotation[2] * M_RADTODEG));
			break;
		case PMX::RigidBodyShape::Capsule:
			collider->SetShapeType(SHAPE_CAPSULE);
			collider->SetCapsule(Body.size[0], Body.size[1], Vector3(Body.position) - Node->GetWorldPosition() + node_->GetWorldPosition(), Quaternion(Body.rotation[0] * M_RADTODEG, Body.rotation[1] * M_RADTODEG, Body.rotation[2] * M_RADTODEG));
			break;
		case PMX::RigidBodyShape::Sphere:
			collider->SetShapeType(SHAPE_SPHERE);
			collider->SetSphere(Body.size[0], Vector3(Body.position) - Node->GetWorldPosition() + node_->GetWorldPosition(), Quaternion(Body.rotation[0] * M_RADTODEG, Body.rotation[1] * M_RADTODEG, Body.rotation[2] * M_RADTODEG));
			break;
		}
	}

	// Create constraints
	for (unsigned int constraintIndex = 0; constraintIndex < model->jointList.Size(); ++constraintIndex)
	{
		auto constraint = model->jointList[constraintIndex];

		auto Node = createdRigidBodies.At(constraint.data.bodyA)->GetNode();
		auto constraintComponent = Node->CreateComponent<Urho3D::Constraint>();
		constraintComponent->SetOtherBody(createdRigidBodies.At(constraint.data.bodyB));
		constraintComponent->SetPosition(Vector3(constraint.data.position) - Node->GetWorldPosition() + node_->GetWorldPosition());
		constraintComponent->SetRotation(Quaternion(constraint.data.rotation[0] * M_RADTODEG, constraint.data.rotation[1] * M_RADTODEG, constraint.data.rotation[2] * M_RADTODEG));

		switch (constraint.type) {
		case PMX::JointType::Hinge:
		{
			constraintComponent->SetConstraintType(CONSTRAINT_HINGE);
			constraintComponent->SetLowLimit(Vector2(M_RADTODEG * constraint.data.lowerRotationRestrictions[0], 0));
			constraintComponent->SetHighLimit(Vector2(M_RADTODEG * constraint.data.upperRotationRestrictions[0], 0));

			/*auto btConstraint = static_cast<btHingeConstraint*>(constraintComponent->GetConstraint());
			btConstraint->setLimit(constraint.data.lowerRotationRestrictions[0], constraint.data.upperRotationRestrictions[0], constraint.data.springConstant[0], constraint.data.springConstant[1], constraint.data.springConstant[2]);

			bool motor = constraint.data.springConstant[3] != 0.0f;
			btConstraint->enableMotor(motor);
			if (motor) {
				btConstraint->enableAngularMotor(motor, constraint.data.springConstant[4], constraint.data.springConstant[5]);
			}*/

			break;
		}
		case PMX::JointType::ConeTwist:
			constraintComponent->SetConstraintType(CONSTRAINT_CONETWIST); break;
		case PMX::JointType::Slider:
			constraintComponent->SetConstraintType(CONSTRAINT_SLIDER); break;
		case PMX::JointType::PointToPoint:
			constraintComponent->SetConstraintType(CONSTRAINT_POINT); break;
		case PMX::JointType::SixDoF:
		{
			constraintComponent->SetConstraintType(CONSTRAINT_GENERIC6DOF);
			constraintComponent->Set6DoFLowerLinearLimit(Vector3(constraint.data.lowerMovementRestrictions));
			constraintComponent->Set6DoFUpperLinearLimit(Vector3(constraint.data.upperMovementRestrictions));
			constraintComponent->Set6DoFLowerAngularLimit(Vector3(constraint.data.lowerRotationRestrictions));
			constraintComponent->Set6DoFUpperAngularLimit(Vector3(constraint.data.upperRotationRestrictions));
			break;
		}
		case PMX::JointType::Spring6DoF:
		{
			constraintComponent->SetConstraintType(CONSTRAINT_GENERIC6DOFSPRING);
			constraintComponent->Set6DoFLowerLinearLimit(Vector3(constraint.data.lowerMovementRestrictions));
			constraintComponent->Set6DoFUpperLinearLimit(Vector3(constraint.data.upperMovementRestrictions));
			constraintComponent->Set6DoFLowerAngularLimit(Vector3(constraint.data.lowerRotationRestrictions));
			constraintComponent->Set6DoFUpperAngularLimit(Vector3(constraint.data.upperRotationRestrictions));

			for (int i = 0; i < 6; i++) {
				if (i >= 3 || constraint.data.springConstant[i] != 0.0f) {
					constraintComponent->SetEnableSpring(i, true);
					constraintComponent->SetSpringStiffness(i, constraint.data.springConstant[i]);
				}
			}

			break;
		}
		}
	}

	Urho3D::ResourceCache* resourceCache = context_->GetSubsystem<Urho3D::ResourceCache>();

	// Create materials
	for (unsigned int materialIndex = 0; materialIndex < model->materialList.Size(); ++materialIndex)
	{
		auto ModelMat = model->materialList[materialIndex];
		Urho3D::Material *material = new Urho3D::Material(context_);

		SharedPtr<Urho3D::Image> base;
		if (ModelMat.baseTexture != -1) {
			base = resourceCache->GetResource<Urho3D::Image>(model->textureList[ModelMat.baseTexture], false);

			if (base != nullptr) {
				Texture2D *baseTex = resourceCache->GetResource<Urho3D::Texture2D>(model->textureList[ModelMat.baseTexture], false);
				baseTex->SetData(base, true);
				baseTex->SetFilterMode(Urho3D::FILTER_BILINEAR);
				material->SetTexture(Urho3D::TU_ALBEDOBUFFER, baseTex);
			}
		}

		if (ModelMat.sphereTexture != -1) {
			SharedPtr<Urho3D::Image> sphereImg;
			String sphereTexName = model->textureList[ModelMat.sphereTexture];
			sphereImg = resourceCache->GetResource<Urho3D::Image>(sphereTexName, false);
			if (sphereImg != nullptr) {
				Texture2D *sphereTex = resourceCache->GetResource<Urho3D::Texture2D>(sphereTexName, false);
				sphereTex->SetData(sphereImg, true);
				sphereTex->SetFilterMode(Urho3D::FILTER_BILINEAR);
				material->SetTexture(Urho3D::TU_ENVIRONMENT, sphereTex);
			}
		}

		if (base != nullptr) {
			if (base->GetComponents() != 3) {
				if (ModelMat.diffuse[3] == 1.0f)
					material->SetTechnique(0, resourceCache->GetResource<Urho3D::Technique>("Techniques/DiffAOAlphaMask.xml"));
				else
					material->SetTechnique(0, resourceCache->GetResource<Urho3D::Technique>("Techniques/DiffAOAlpha.xml"));
			}
			else material->SetTechnique(0, resourceCache->GetResource<Urho3D::Technique>("Techniques/DiffAO.xml"));
		}
		else {
			if (ModelMat.diffuse[3] != 1.0f) {
				material->SetTechnique(0, resourceCache->GetResource<Urho3D::Technique>("Techniques/NoTextureAOAlpha.xml"));
			}
			else material->SetTechnique(0, resourceCache->GetResource<Urho3D::Technique>("Techniques/NoTextureAO.xml"));
		}
		material->SetShaderParameter("MatDiffColor", Vector4(ModelMat.diffuse));
		material->SetShaderParameter("MatEnvMapColor", Vector3(ModelMat.ambient));
		material->SetShaderParameter("MatSpecColor", Vector4(ModelMat.specular[0], ModelMat.specular[1], ModelMat.specular[2], ModelMat.specularCoefficient));
		material->SetName(model->GetDescription().name.japanese + "_mat" + String(materialIndex) + ".xml");
		if (ModelMat.flags & (unsigned char)PMX::MaterialFlags::DoubleSide)
			material->SetCullMode(CULL_NONE);
		this->SetMaterial(materialIndex, material);
	}

	{
		File f(context_, name + "-prefab.xml", FILE_WRITE);
		this->GetNode()->SaveXML(f);
		f.Close();
	}
}
