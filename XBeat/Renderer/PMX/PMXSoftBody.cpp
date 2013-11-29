#include "PMXSoftBody.h"
#include "PMXModel.h"

#include "BulletSoftBody/btSoftBodyHelpers.h"

using namespace Renderer::PMX;

SoftBody::SoftBody(void)
{
}


SoftBody::~SoftBody(void)
{
}

void SoftBody::Create(std::shared_ptr<Physics::Environment> physics)
{
	/*btSoftBody *body;
	switch (this->shape) {
	case Shape::TriangleMesh:
		body = btSoftBodyHelpers::CreateFromTriMesh(physics->GetWorldInfo());
		break;
	case Shape::Rope:
		body = btSoftBodyHelpers::CreateRope(physics->GetWorldInfo(), 
	}*/
}
