#include "PMXAnimatedModel.h"
#include "PMXIKNode.h"
#include "PMXIKSolver.h"
#include "PMXModel.h"
#include "PMXRigidBody.h"

#include <Urho3D/Core/Context.h>
#include <Urho3D/IO/File.h>
#include <Urho3D/Graphics/GraphicsDefs.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Technique.h>
#include <Urho3D/Graphics/Texture2D.h>
#include <Urho3D/Physics/CollisionShape.h>
#include <Urho3D/Physics/Constraint.h>
#include <Urho3D/Physics/RigidBody.h>
#include <Urho3D/Resource/Image.h>
#include <Urho3D/Resource/JSONFile.h>
#include <Urho3D/Resource/ResourceCache.h>

using namespace Urho3D;

void PMXAnimatedModel::SetModel(PMXModel *model, AnimatedModel *animModel, const String &baseDir)
{
	if (!animModel || !model)
		return;

	auto name = model->GetName();
	model->SetName(name);

	animModel->SetModel(model, true);

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
		node = animModel->GetSkeleton().GetBone(i)->node_;

		Matrix3x4 offset = Matrix3x4::IDENTITY;
		int current = i;
		for (Bone* b = animModel->GetSkeleton().GetBone(current); b != NULL && b->node_ != NULL && b->node_->GetParent() != animModel->GetNode(); b = animModel->GetSkeleton().GetBone(current)) {
			offset = offset * Matrix3x4(b->initialPosition_, b->initialRotation_, b->initialScale_);

			current = b->parentIndex_;
		}
        animModel->GetSkeleton().GetBone(i)->offsetMatrix_ = offset.Inverse();

		auto ikTarget = node->CreateComponent<PMXIKSolver>();
		ikTarget->SetAngleLimit(bone.IkData.angleLimit);
		ikTarget->SetLoopCount(bone.IkData.loopCount);
		auto endNode = animModel->GetSkeleton().GetBone(bone.IkData.targetIndex)->node_;
		ikTarget->AddNode(endNode->CreateComponent<PMXIKNode>());
		ikTarget->SetEndNode(endNode);

		for (auto it = bone.IkData.links.Begin(); it != bone.IkData.links.End(); ++it) {
			auto boneNode = animModel->GetSkeleton().GetBone(it->boneIndex)->node_;
			auto ikNode = boneNode->CreateComponent<PMXIKNode>();
			ikNode->SetBoneLength(getBoneLength(model->boneList, model->boneList[it->boneIndex]));
			ikNode->SetLimited(it->limitAngle);
			ikNode->SetLowerLimit(Vector3(it->limits.lower));
			ikNode->SetUpperLimit(Vector3(it->limits.upper));
			ikNode->SetBone(animModel->GetSkeleton().GetBone(it->boneIndex));
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
			Node = animModel->GetSkeleton().GetRootBone()->node_;
		else
			Node = animModel->GetSkeleton().GetBone(Body.targetBone)->node_;

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

		Vector3 positionOffset = Vector3(Body.position) - Node->GetWorldPosition() + animModel->GetNode()->GetWorldPosition();
		switch (Body.shape) {
		case PMX::RigidBodyShape::Box:
			collider->SetShapeType(SHAPE_BOX);
			collider->SetBox(Vector3(Body.size), positionOffset, Quaternion(Body.rotation[0] * M_RADTODEG, Body.rotation[1] * M_RADTODEG, Body.rotation[2] * M_RADTODEG));
			break;
		case PMX::RigidBodyShape::Capsule:
			collider->SetShapeType(SHAPE_CAPSULE);
			collider->SetCapsule(Body.size[0], Body.size[1], Vector3(Body.position) - Node->GetWorldPosition() + animModel->GetNode()->GetWorldPosition(), Quaternion(Body.rotation[0] * M_RADTODEG, Body.rotation[1] * M_RADTODEG, Body.rotation[2] * M_RADTODEG));
			break;
		case PMX::RigidBodyShape::Sphere:
			collider->SetShapeType(SHAPE_SPHERE);
			collider->SetSphere(Body.size[0], Vector3(Body.position) - Node->GetWorldPosition() + animModel->GetNode()->GetWorldPosition(), Quaternion(Body.rotation[0] * M_RADTODEG, Body.rotation[1] * M_RADTODEG, Body.rotation[2] * M_RADTODEG));
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
		constraintComponent->SetPosition(Vector3(constraint.data.position) - Node->GetWorldPosition() + animModel->GetNode()->GetWorldPosition());
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
			constraintComponent->SetLowLinearLimit(Vector3(constraint.data.lowerMovementRestrictions));
			constraintComponent->SetHighLinearLimit(Vector3(constraint.data.upperMovementRestrictions));
			constraintComponent->SetLowAngularLimit(Vector3(constraint.data.lowerRotationRestrictions));
			constraintComponent->SetHighAngularLimit(Vector3(constraint.data.upperRotationRestrictions));
			break;
		}
		case PMX::JointType::Spring6DoF:
		{
			constraintComponent->SetConstraintType(CONSTRAINT_GENERIC6DOFSPRING);
			constraintComponent->SetLowLinearLimit(Vector3(constraint.data.lowerMovementRestrictions));
			constraintComponent->SetHighLinearLimit(Vector3(constraint.data.upperMovementRestrictions));
			constraintComponent->SetLowAngularLimit(Vector3(constraint.data.lowerRotationRestrictions));
			constraintComponent->SetHighAngularLimit(Vector3(constraint.data.upperRotationRestrictions));

            constraintComponent->SetLinearStiffness(Vector3(constraint.data.springConstant));
            constraintComponent->SetAngularStiffness(Vector3(constraint.data.springConstant + 3));

			break;
		}
		}
	}

	Urho3D::ResourceCache* resourceCache = animModel->GetContext()->GetSubsystem<Urho3D::ResourceCache>();

	// Create materials
	for (unsigned int materialIndex = 0; materialIndex < model->materialList.Size(); ++materialIndex)
	{
		auto ModelMat = model->materialList[materialIndex];
		Urho3D::Material *material = new Urho3D::Material(animModel->GetContext());
        Texture2D *baseTex = nullptr;

		if (ModelMat.baseTexture != -1) {
            baseTex = resourceCache->GetResource<Urho3D::Texture2D>(baseDir + model->textureList[ModelMat.baseTexture]);
            if (baseTex != nullptr) {
				material->SetTexture(Urho3D::TU_ALBEDOBUFFER, baseTex);
			}
		}

		if (ModelMat.sphereTexture != -1) {
			String sphereTexName = baseDir + model->textureList[ModelMat.sphereTexture];
            Texture2D *sphereTex = resourceCache->GetResource<Urho3D::Texture2D>(sphereTexName, false);
            if (sphereTex != nullptr) {
				material->SetTexture(Urho3D::TU_ENVIRONMENT, sphereTex);
			}
		}

        if (baseTex != nullptr) {
			if (baseTex->GetComponents() != 3) {
				if (ModelMat.diffuse[3] == 1.0f)
					material->SetTechnique(0, resourceCache->GetResource<Urho3D::Technique>("Techniques/DiffAlphaMask.xml"));
				else
					material->SetTechnique(0, resourceCache->GetResource<Urho3D::Technique>("Techniques/DiffAlpha.xml"));
			}
			else material->SetTechnique(0, resourceCache->GetResource<Urho3D::Technique>("Techniques/Diff.xml"));
		}
		else {
			if (ModelMat.diffuse[3] != 1.0f) {
				material->SetTechnique(0, resourceCache->GetResource<Urho3D::Technique>("Techniques/NoTextureAlpha.xml"));
			}
			else material->SetTechnique(0, resourceCache->GetResource<Urho3D::Technique>("Techniques/NoTexture.xml"));
		}
		material->SetShaderParameter("MatDiffColor", Vector4(ModelMat.diffuse));
		material->SetShaderParameter("MatEnvMapColor", Vector3(ModelMat.ambient));
		material->SetShaderParameter("MatSpecColor", Vector4(ModelMat.specular[0], ModelMat.specular[1], ModelMat.specular[2], ModelMat.specularCoefficient));
		material->SetName(model->GetDescription().name.japanese + "_mat" + String(materialIndex) + ".json");
		if (ModelMat.flags & (unsigned char)PMX::MaterialFlags::DoubleSide)
			material->SetCullMode(CULL_NONE);
		animModel->SetMaterial(materialIndex, material);
	}
}
