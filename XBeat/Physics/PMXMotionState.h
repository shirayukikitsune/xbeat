#pragma once

#include "LinearMath/btMotionState.h"
#include "LinearMath/btTransform.h"

namespace PMX
{
	class Bone;

	class PMXMotionState
		: public btMotionState
	{
	public:
		PMXMotionState(Bone* bone, const btTransform &initialTransform);
		virtual ~PMXMotionState();

		virtual void getWorldTransform(btTransform &worldTrans) const;

		virtual void setWorldTransform(const btTransform &worldTrans);

	private:
		Bone *m_bone;
		btTransform m_transform;
	};

}
