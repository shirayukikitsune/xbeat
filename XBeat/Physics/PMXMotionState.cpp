#include "../PMX/PMXBone.h"
#include "PMXMotionState.h"

using namespace PMX;

PMXMotionState::PMXMotionState(Bone *bone, const btTransform &initialTransform)
{
	m_transform = initialTransform;
	m_bone = bone;
}

PMXMotionState::~PMXMotionState()
{
}

void PMXMotionState::getWorldTransform(btTransform &worldTrans) const
{
	worldTrans = m_transform * m_bone->GetLocalTransform();
}

void PMXMotionState::setWorldTransform(const btTransform &transform)
{
}
