#pragma once

#include <Urho3D/Urho3D.h>
#include <Urho3D/Graphics/AnimatedModel.h>

class PMXModel;

class PMXAnimatedModel :
	public Urho3D::AnimatedModel
{
	OBJECT(PMXAnimatedModel);

public:
	PMXAnimatedModel(Urho3D::Context *context);
	virtual ~PMXAnimatedModel();
	/// Register object factory. Drawable must be registered first.
	static void RegisterObject(Urho3D::Context* context);

	void SetModel(PMXModel *model);

protected:
	/// Handle node being assigned.
	virtual void OnNodeSet(Urho3D::Node* node);
};

