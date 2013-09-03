#include "ViewFrustum.h"

using Renderer::ViewFrustum;

ViewFrustum::ViewFrustum(void)
{
	for (auto &i : planes)
		i = DirectX::XMVectorZero();
}


ViewFrustum::~ViewFrustum(void)
{
}

void ViewFrustum::Construct(float depth, DirectX::CXMMATRIX projection, DirectX::CXMMATRIX view)
{
	float minZ, r;
	DirectX::XMFLOAT4 tmp;
	DirectX::XMMATRIX matrix, projCopy;

	// Find the minimum Z distance
	minZ = - projection.r[3].m128_f32[2] / projection.r[2].m128_f32[2];
	r = depth / (depth - minZ);
	projCopy = projection;
	projCopy.r[2].m128_f32[2] = r;
	projCopy.r[3].m128_f32[2] = -r * minZ;

	matrix = DirectX::XMMatrixMultiply(view, projCopy);

	// Calculate each plane
	tmp.x = matrix.r[0].m128_f32[3] + matrix.r[0].m128_f32[2];
	tmp.y = matrix.r[1].m128_f32[3] + matrix.r[1].m128_f32[2];
	tmp.z = matrix.r[2].m128_f32[3] + matrix.r[2].m128_f32[2];
	tmp.w = matrix.r[3].m128_f32[3] + matrix.r[3].m128_f32[2];
	planes[0] = DirectX::XMPlaneNormalize(DirectX::XMLoadFloat4(&tmp));

	tmp.x = matrix.r[0].m128_f32[3] - matrix.r[0].m128_f32[2];
	tmp.y = matrix.r[1].m128_f32[3] - matrix.r[1].m128_f32[2];
	tmp.z = matrix.r[2].m128_f32[3] - matrix.r[2].m128_f32[2];
	tmp.w = matrix.r[3].m128_f32[3] - matrix.r[3].m128_f32[2];
	planes[1] = DirectX::XMPlaneNormalize(DirectX::XMLoadFloat4(&tmp));

	tmp.x = matrix.r[0].m128_f32[3] + matrix.r[0].m128_f32[0];
	tmp.y = matrix.r[1].m128_f32[3] + matrix.r[1].m128_f32[0];
	tmp.z = matrix.r[2].m128_f32[3] + matrix.r[2].m128_f32[0];
	tmp.w = matrix.r[3].m128_f32[3] + matrix.r[3].m128_f32[0];
	planes[2] = DirectX::XMPlaneNormalize(DirectX::XMLoadFloat4(&tmp));

	tmp.x = matrix.r[0].m128_f32[3] - matrix.r[0].m128_f32[0];
	tmp.y = matrix.r[1].m128_f32[3] - matrix.r[1].m128_f32[0];
	tmp.z = matrix.r[2].m128_f32[3] - matrix.r[2].m128_f32[0];
	tmp.w = matrix.r[3].m128_f32[3] - matrix.r[3].m128_f32[0];
	planes[3] = DirectX::XMPlaneNormalize(DirectX::XMLoadFloat4(&tmp));

	tmp.x = matrix.r[0].m128_f32[3] - matrix.r[0].m128_f32[1];
	tmp.y = matrix.r[1].m128_f32[3] - matrix.r[1].m128_f32[1];
	tmp.z = matrix.r[2].m128_f32[3] - matrix.r[2].m128_f32[1];
	tmp.w = matrix.r[3].m128_f32[3] - matrix.r[3].m128_f32[1];
	planes[4] = DirectX::XMPlaneNormalize(DirectX::XMLoadFloat4(&tmp));

	tmp.x = matrix.r[0].m128_f32[3] + matrix.r[0].m128_f32[1];
	tmp.y = matrix.r[1].m128_f32[3] + matrix.r[1].m128_f32[1];
	tmp.z = matrix.r[2].m128_f32[3] + matrix.r[2].m128_f32[1];
	tmp.w = matrix.r[3].m128_f32[3] + matrix.r[3].m128_f32[1];
	planes[5] = DirectX::XMPlaneNormalize(DirectX::XMLoadFloat4(&tmp));
}


bool ViewFrustum::IsPointInside(DirectX::XMFLOAT3 point)
{
	float dot;

	for (int i = 0; i < 6; i++) {
		DirectX::XMStoreFloat(&dot, DirectX::XMPlaneDotCoord(planes[i], DirectX::XMLoadFloat3(&point)));
		if (dot < 0.0f)
			return true;
	}

	return false;
}

bool ViewFrustum::IsPointInside(float x, float y, float z)
{
	return IsPointInside(DirectX::XMFLOAT3(x, y, z));
}

bool ViewFrustum::IsCubeInside(DirectX::XMFLOAT3 center, float radius)
{
	DirectX::XMFLOAT3 r(radius, radius, radius);
	return IsBoxInside(center, r);
}

bool ViewFrustum::IsCubeInside(float x, float y, float z, float radius)
{
	return IsCubeInside(DirectX::XMFLOAT3(x, y, z), radius);
}

bool ViewFrustum::IsSphereInside(DirectX::XMFLOAT3 center, float radius)
{
	float dot;

	for (int i = 0; i < 6; i++) {
		DirectX::XMStoreFloat(&dot, DirectX::XMPlaneDotCoord(planes[i], DirectX::XMLoadFloat3(&center)));
		if (dot < -radius)
			return true;
	}

	return false;
}

bool ViewFrustum::IsSphereInside(float x, float y, float z, float radius)
{
	return IsSphereInside(DirectX::XMFLOAT3(x, y, z), radius);
}

bool ViewFrustum::IsBoxInside(DirectX::XMFLOAT3 center, DirectX::XMFLOAT3 radius)
{
	float dot;
	DirectX::XMFLOAT3 tmp;

	for (int i = 0; i < 6; i++) {
		tmp.x = center.x - radius.x;
		tmp.y = center.x - radius.x;
		tmp.z = center.x - radius.x;
		DirectX::XMStoreFloat(&dot, DirectX::XMPlaneDotCoord(planes[i], DirectX::XMLoadFloat3(&tmp)));
		if (dot >= 0.0f)
			return true;

		tmp.x = center.x + radius.x;
		tmp.y = center.y - radius.y;
		tmp.z = center.z - radius.z;
		DirectX::XMStoreFloat(&dot, DirectX::XMPlaneDotCoord(planes[i], DirectX::XMLoadFloat3(&tmp)));
		if (dot >= 0.0f)
			return true;

		tmp.x = center.x - radius.x;
		tmp.y = center.y + radius.y;
		tmp.z = center.z - radius.z;
		DirectX::XMStoreFloat(&dot, DirectX::XMPlaneDotCoord(planes[i], DirectX::XMLoadFloat3(&tmp)));
		if (dot >= 0.0f)
			return true;

		tmp.x = center.x - radius.x;
		tmp.y = center.y - radius.y;
		tmp.z = center.z + radius.z;
		DirectX::XMStoreFloat(&dot, DirectX::XMPlaneDotCoord(planes[i], DirectX::XMLoadFloat3(&tmp)));
		if (dot >= 0.0f)
			return true;

		tmp.x = center.x + radius.x;
		tmp.y = center.y + radius.y;
		tmp.z = center.z - radius.z;
		DirectX::XMStoreFloat(&dot, DirectX::XMPlaneDotCoord(planes[i], DirectX::XMLoadFloat3(&tmp)));
		if (dot >= 0.0f)
			return true;

		tmp.x = center.x + radius.x;
		tmp.y = center.y - radius.y;
		tmp.z = center.z + radius.z;
		DirectX::XMStoreFloat(&dot, DirectX::XMPlaneDotCoord(planes[i], DirectX::XMLoadFloat3(&tmp)));
		if (dot >= 0.0f)
			return true;

		tmp.x = center.x - radius.x;
		tmp.y = center.y + radius.y;
		tmp.z = center.z + radius.z;
		DirectX::XMStoreFloat(&dot, DirectX::XMPlaneDotCoord(planes[i], DirectX::XMLoadFloat3(&tmp)));
		if (dot >= 0.0f)
			return true;

		tmp.x = center.x + radius.x;
		tmp.y = center.y + radius.y;
		tmp.z = center.z + radius.z;
		DirectX::XMStoreFloat(&dot, DirectX::XMPlaneDotCoord(planes[i], DirectX::XMLoadFloat3(&tmp)));
		if (dot >= 0.0f)
			return true;
	}

	return false;
}

bool ViewFrustum::IsBoxInside(float x, float y, float z, float rx, float ry, float rz)
{
	return IsBoxInside(DirectX::XMFLOAT3(x, y, z), DirectX::XMFLOAT3(rx, ry, rz));
}

