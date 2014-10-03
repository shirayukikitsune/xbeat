#pragma once

#include "Entity.h"

namespace Renderer {

class Camera
	: public Entity
{
public:
	Camera();

	virtual bool Update(float msec);

	void XM_CALLCONV GetViewMatrix(DirectX::XMMATRIX &matrix);

private:
	DirectX::XMMATRIX m_viewMatrix;
};

}
