//===-- Physics/PMXMotionState.h - Declares a Motion State for PMX ----*- C++ -*-===//
//
//                      The XBeat Project
//
// This file is distributed under the University of Illinois Open Source License.
// See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------------===//
///
/// \file
/// \brief This file declares everything related to the Physics::PMXMotionState class,
/// which is an interface between bullet dynamics world and a PMX::Model.
///
//===----------------------------------------------------------------------------===//

#pragma once

#include "LinearMath/btMotionState.h"
#include "LinearMath/btTransform.h"

namespace PMX { class Bone; }

namespace Physics
{
	/// \brief The PMX::Model and Bullet communication interface
	class PMXMotionState
		: public btMotionState
	{
	public:
		/// \brief Constructs this Motion State
		///
		/// \param [in] AssociatedBone The PMX::Bone that this motion state is associated with
		/// \param [in] InitialTransform The initial transform of the associated rigid body
		PMXMotionState(PMX::Bone* AssociatedBone, const btTransform &InitialTransform);

		/// \brief This function is called by the Bullet library in order to update the rigid body in the simulation
		virtual void getWorldTransform(btTransform &WorldTransform) const;

		/// \brief This function does nothing, since this class is used to update the rigid body when the bone is deformed and is not affected by the physics world
		virtual void setWorldTransform(const btTransform &WorldTransform);

#if defined _M_IX86 && defined _MSC_VER
		void *__cdecl operator new(size_t count){
			return _aligned_malloc(count, 16);
		}

			void __cdecl operator delete(void *object) {
			_aligned_free(object);
		}
#endif

	private:
		PMX::Bone *AssociatedBone;
		btTransform InitialTransform;
	};

}
