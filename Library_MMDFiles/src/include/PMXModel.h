#pragma once

#include "BulletPhysics.h"
#include "SystemTexture.h"
#include <cstdint>

struct PMX_Header
{
	char magic[4];
	float version;
};

struct PMX_SizeInfo
{
	uint8_t cbSize;
	uint8_t cbEncoding;
	uint8_t cbUVVectorSize;
	uint8_t cbVertexIndexSize;
	uint8_t cbTextureIndexSize;
	uint8_t cbMaterialIndexSize;
	uint8_t cbBoneIndexSize;
	uint8_t cbMorphIndexSize;
	uint8_t cbRigidBodyIndexSize;
};

struct PMX_Vertex
{
	float fvPosition[3];
	float fvNormal[3];
	float fvUV[2];
};

struct PMX_VertexEx
{
	PMX_Vertex *pVertexInfo;
	float (*fvUVextra)[4];
	uint8_t cbWeightType;
};

class PMXModel
{
public:
	PMXModel(void);
	~PMXModel(void);

	bool parse(const unsigned char *data, unsigned long size, BulletPhysics *bullet, SystemTexture *systex, const char *dir);
};

