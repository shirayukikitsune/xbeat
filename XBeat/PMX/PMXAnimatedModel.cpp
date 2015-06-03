#include "PMXAnimatedModel.h"
#include "PMXModel.h"

#include <CollisionShape.h>
#include <Constraint.h>
#include <Context.h>
#include <Image.h>
#include <Material.h>
#include <ResourceCache.h>
#include <RigidBody.h>
#include <Technique.h>
#include <Texture2D.h>

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

	AnimatedModel::SetModel(model, true);

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
			collider->SetBox(Vector3(Body.size), positionOffset, Quaternion(Body.rotation[0], Body.rotation[1], Body.rotation[2]));
			break;
		case PMX::RigidBodyShape::Capsule:
			collider->SetShapeType(SHAPE_CAPSULE);
			collider->SetCapsule(Body.size[0], Body.size[1], Vector3(Body.position) - Node->GetWorldPosition() + node_->GetWorldPosition(), Quaternion(Body.rotation[0], Body.rotation[1], Body.rotation[2]));
			break;
		case PMX::RigidBodyShape::Sphere:
			collider->SetShapeType(SHAPE_SPHERE);
			collider->SetSphere(Body.size[0], Vector3(Body.position) - Node->GetWorldPosition() + node_->GetWorldPosition(), Quaternion(Body.rotation[0], Body.rotation[1], Body.rotation[2]));
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
		constraintComponent->SetRotation(Quaternion(constraint.data.rotation[0] * M_DEGTORAD, constraint.data.rotation[1] * M_DEGTORAD, constraint.data.rotation[2] * M_DEGTORAD));

		switch (constraint.type) {
		case PMX::JointType::Hinge:
			constraintComponent->SetConstraintType(CONSTRAINT_HINGE); break;
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
		material->SetName(model->GetDescription().name.japanese + "_mat" + String(materialIndex));
		if (ModelMat.flags & (unsigned char)PMX::MaterialFlags::DoubleSide)
			material->SetCullMode(CULL_NONE);
		this->SetMaterial(materialIndex, material);
	}

}
