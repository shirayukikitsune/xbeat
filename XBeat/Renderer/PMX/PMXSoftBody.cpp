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

void SoftBody::Create(std::shared_ptr<Physics::Environment> physics, Model* model)
{
	btSoftBody *body;
	btScalar *vertices;
	int *triangles;
	
	auto material = model->GetMaterialById(this->material);
	triangles = new int[material->indexCount];
	vertices = new btScalar[material->indexCount / 3];

	/*switch (this->shape) {
	case Shape::TriangleMesh:
		body = btSoftBodyHelpers::CreateFromTriMesh(physics->GetWorldInfo(), vertices, triangles, model->GetMaterialById(this->material)->indexCount);
		break;
	case Shape::Rope:
		body = btSoftBodyHelpers::CreateRope(physics->GetWorldInfo(), );
		break;
	}*/
	//body->m_faces[0].

	delete[] triangles;
	delete[] vertices;
}
