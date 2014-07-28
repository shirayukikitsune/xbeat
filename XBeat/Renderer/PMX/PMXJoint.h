#pragma once

#include "PMXDefinitions.h"
#include "PMXLoader.h"

namespace Renderer {
namespace PMX {

class Model;

ATTRIBUTE_ALIGNED16(class) Joint
{
public:
	Joint();
	~Joint();
	BT_DECLARE_ALIGNED_ALLOCATOR();

	bool Initialize(std::shared_ptr<Physics::Environment> physics, PMX::Model *model, Loader::Joint *joint);
	void Shutdown(std::shared_ptr<Physics::Environment> physics);

private:
	std::shared_ptr<btTypedConstraint> m_constraint;
};

}
}

