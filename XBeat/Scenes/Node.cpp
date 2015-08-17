#include "Node.h"

Node::Node(Node *Parent)
	: Parent(Parent)
{
	Position = DirectX::XMVectorZero();
	Rotation = DirectX::XMQuaternionIdentity();
	LocalTransform = WorldTransform = DirectX::XMMatrixIdentity();
}

Node::~Node()
{
}
