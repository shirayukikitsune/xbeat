#include "stdafx.h"
#include "CppUnitTest.h"
#include "PMXModel.h"

#include "Motion.h"

#include <sys/stat.h>
#include <cmath>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using PMX::Model;
using VMD::Motion;

namespace Microsoft{ namespace VisualStudio {namespace CppUnitTestFramework{
	template<> static std::wstring ToString<Model> (const Model &m) {
		RETURN_WIDE_STRING(L"JPname: "<< m.description.nameJP << L" ENname: " << m.description.nameEN << L" JPdesc: "<< m.description.commentJP << L" ENdesc: " << m.description.commentEN);
	}
	template<> static std::wstring ToString<Model> (const Model *m) {
		if (m == nullptr) { RETURN_WIDE_STRING(L"(nullptr)"); };

		return ToString<Model>(*m);
	}
	template<> static std::wstring ToString<Model> (Model *m) {
		if (m == nullptr) { RETURN_WIDE_STRING(L"(nullptr)"); };

		return ToString<Model>(*m);
	}

	template<> static std::wstring ToString<Motion> (const Motion &m) {
		RETURN_WIDE_STRING(L"keyframe: " << m.GetKeyFrameCount() << L" faceKeyFrame: " << m.GetFaceKeyFrameCount());
	}
	template<> static std::wstring ToString<Motion> (const Motion *m) {
		if (m == nullptr) { RETURN_WIDE_STRING(L"(nullptr)"); };

		return ToString<Motion>(*m);
	}
	template<> static std::wstring ToString<Motion> (Motion *m) {
		if (m == nullptr) { RETURN_WIDE_STRING(L"(nullptr)"); };

		return ToString<Motion>(*m);
	}
}}}

namespace PMXTest
{		
	TEST_CLASS(UnitTest1)
	{
	private:
		Model *createModel() {
			Model *model = nullptr;

			if (model == nullptr) {
				model = new Model();
				Assert::AreNotEqual<Model*>(nullptr, model);
				Assert::IsTrue(model->loadModel(L"C:/Users/sakagushi/Documents/GitHub/xbeat/Debug/AppData/BeatMap/Models/Tda_luka_NAL76HN06/luka_Ln76_n19_06.pmx"));
			}

			return model;
		}

		Motion * createMotion() {
			Motion *motion = nullptr;

			if (motion == nullptr) {
				motion = new Motion;
				Assert::AreNotEqual<Motion*>(nullptr, motion);
				Assert::IsTrue(motion->Load(L"C:/Users/sakagushi/Documents/GitHub/xbeat/Debug/AppData/BeatMap/Musics/Yellow/YellowCamera.vmd"));
			}

			return motion;
		}

	public:
		TEST_METHOD(KeyFrameCount)
		{
			Motion *m = createMotion();

			Assert::AreEqual(90U, m->GetKeyFrameCount());
		}

#define IS_RANGE(x, i, a) Assert::IsTrue((x) >= i && (x) <= a)
#define IS_124(x) Assert::IsTrue((x) == 1 || (x) == 2 || (x) == 4)
		
		TEST_METHOD(HeaderTest)
		{
			Model *m = createModel();
			Assert::AreEqual("PMX ", m->header->abMagic);
			Assert::AreEqual(2.0f, m->header->fVersion);
		}

		TEST_METHOD(SizeInfoTest)
		{
			Model *m = createModel();
			Assert::AreEqual(8, (int)m->sizeInfo->cbSize);
			Assert::AreEqual(0, (int)m->sizeInfo->cbEncoding);
			Assert::AreEqual(0, (int)m->sizeInfo->cbUVVectorSize);
			Assert::AreEqual(2, (int)m->sizeInfo->cbVertexIndexSize);
			Assert::AreEqual(1, (int)m->sizeInfo->cbTextureIndexSize);
			Assert::AreEqual(1, (int)m->sizeInfo->cbMaterialIndexSize);
			Assert::AreEqual(2, (int)m->sizeInfo->cbBoneIndexSize);
			Assert::AreEqual(1, (int)m->sizeInfo->cbMorphIndexSize);
			Assert::AreEqual(1, (int)m->sizeInfo->cbRigidBodyIndexSize);
		}

		TEST_METHOD(DescriptionTest)
		{
			Model *m = createModel();
			Assert::AreEqual(0x16, (int)(m->description.nameEN.size() * sizeof(std::wstring::value_type)));
			Assert::AreEqual(L"MIKU Append", m->description.nameEN.c_str());
		}

		TEST_METHOD(VertexTest)
		{
			Model *m = createModel();
			Assert::AreEqual(31170, (int)m->vertices.size());
			PMX::Vertex *v = m->vertices[9];
			Assert::AreEqual(1.0067f, v->position.x, 0.0001f);
			Assert::AreEqual(18.16053f, v->position.y, 0.0001f);
			Assert::AreEqual(-0.2237031f, v->position.z, 0.0001f);
			Assert::AreEqual(0.6185657f, v->normal.x, 0.0001f);
			Assert::AreEqual(-0.2522425f, v->normal.y, 0.0001f);
			Assert::AreEqual(-0.7441441f, v->normal.z, 0.0001f);
			Assert::AreEqual(0.21362f, v->uv[0], 0.0001f);
			Assert::AreEqual(0.26588f, v->uv[1], 0.0001f);
			Assert::AreEqual(0, (int)v->weightMethod);
			Assert::AreEqual(185U, v->boneInfo.BDEF.boneIndexes[0]);
			Assert::AreEqual(1.0f, v->edgeWeight);

			v = m->vertices[18145];
			Assert::AreEqual(-1.106945f, v->position.x, 0.0001f);
			Assert::AreEqual(18.5335f, v->position.y, 0.0001f);
			Assert::AreEqual(0.09722519f, v->position.z, 0.0001f);
			Assert::AreEqual(0.2495605f, v->normal.x, 0.0001f);
			Assert::AreEqual(-0.4475462f, v->normal.y, 0.0001f);
			Assert::AreEqual(-0.8587328f, v->normal.z, 0.0001f);
			Assert::AreEqual(0.69952f, v->uv[0], 0.0001f);
			Assert::AreEqual(0.10859f, v->uv[1], 0.0001f);
			Assert::AreEqual(3, (int)v->weightMethod);
			Assert::AreEqual(15U, v->boneInfo.SDEF.boneIndexes[0]);
			Assert::AreEqual(135U, v->boneInfo.SDEF.boneIndexes[1]);
			Assert::AreEqual(0.7272338f, v->boneInfo.SDEF.weightBias, 0.0001f);
			Assert::AreEqual(-1.06345f, v->boneInfo.SDEF.C.x, 0.0001f);
			Assert::AreEqual(18.37989f, v->boneInfo.SDEF.C.y, 0.0001f);
			Assert::AreEqual(0.3651031f, v->boneInfo.SDEF.C.z, 0.0001f);
			Assert::AreEqual(-1.310129f, v->boneInfo.SDEF.R0.x, 0.0001f);
			Assert::AreEqual(18.50657f, v->boneInfo.SDEF.R0.y, 0.0001f);
			Assert::AreEqual(0.4777953f, v->boneInfo.SDEF.R0.z, 0.0001f);
			Assert::AreEqual(-0.7300462f, v->boneInfo.SDEF.R1.x, 0.0001f);
			Assert::AreEqual(18.20868f, v->boneInfo.SDEF.R1.y, 0.0001f);
			Assert::AreEqual(0.2127922f, v->boneInfo.SDEF.R1.z, 0.0001f);
			Assert::AreEqual(1.0f, v->edgeWeight);
		}

		TEST_METHOD(SurfaceTest)
		{
			Model *m = createModel();

			Assert::AreEqual(40148, (int)m->verticesIndex.size() / 3);
		}

		TEST_METHOD(TextureTest)
		{
			Model *m = createModel();

			Assert::AreEqual(15, (int)m->textures.size());
			Assert::AreEqual(L"テクスチャ\\boots.tga", m->textures[6].c_str());
		}

		TEST_METHOD(MaterialTest)
		{
			Model *m = createModel();

			Assert::AreEqual(58, (int)m->materials.size());
			PMX::Material *mat = m->materials[12];
			Assert::AreEqual(L"パンツ", mat->nameJP.c_str());
			Assert::AreEqual(1.0f, mat->diffuse.red, 0.0001f);
			Assert::AreEqual(1.0f, mat->diffuse.green, 0.0001f);
			Assert::AreEqual(1.0f, mat->diffuse.blue, 0.0001f);
			Assert::AreEqual(1.0f, mat->diffuse.alpha, 0.0001f);
			Assert::AreEqual(0.0f, mat->specular.red, 0.0001f);
			Assert::AreEqual(0.0f, mat->specular.green, 0.0001f);
			Assert::AreEqual(0.0f, mat->specular.blue, 0.0001f);
			Assert::AreEqual(50.0f, mat->specularCoefficient, 0.0001f);
			Assert::AreEqual(0.5f, mat->ambient.red, 0.0001f);
			Assert::AreEqual(0.5f, mat->ambient.green, 0.0001f);
			Assert::AreEqual(0.5f, mat->ambient.blue, 0.0001f);
			Assert::AreEqual((uint8_t)(PMX::MaterialFlags::DrawSelfShadowMap | PMX::MaterialFlags::DrawSelfShadow | PMX::MaterialFlags::GroundShadow | PMX::MaterialFlags::DrawEdge), mat->flags);
			Assert::AreEqual(0.45f, mat->edgeColor.red, 0.0001f);
			Assert::AreEqual(0.2f, mat->edgeColor.green, 0.0001f);
			Assert::AreEqual(0.3f, mat->edgeColor.blue, 0.0001f);
			Assert::AreEqual(0.6f, mat->edgeColor.alpha, 0.0001f);
			Assert::AreEqual(0.6f, mat->edgeSize, 0.0001f);
			Assert::AreEqual(L"テクスチャ\\body00_ruka_Ln72.tga", m->textures[mat->baseTexture].c_str());
			Assert::AreEqual(L"テクスチャ\\sph\\body00_s.bmp", m->textures[mat->sphereTexture].c_str());
			Assert::AreEqual((uint8_t)PMX::MaterialSphereMode::Add, mat->sphereMode);
			Assert::AreEqual(0, (int)mat->toonFlag);
			Assert::AreEqual(L"テクスチャ\\toon_defo.bmp", m->textures[mat->toonTexture.custom].c_str());
		}

		TEST_METHOD(BoneTest)
		{
			Model *m = createModel();

			Assert::AreEqual(227, (int)m->bones.size());
			PMX::Bone *b = m->bones[14];
			Assert::AreEqual(L"首", b->nameJP.c_str());
			Assert::AreEqual(0.0f, b->position.x, 0.0001f);
			Assert::AreEqual(16.95938f, b->position.y, 0.0001f);
			Assert::AreEqual(-0.1121041f, b->position.z, 0.0001f);
			Assert::AreEqual(8U, b->parent);
			Assert::AreEqual(0, b->deformation);
			Assert::AreEqual(PMX::BoneFlags::Attached | PMX::BoneFlags::Rotatable | PMX::BoneFlags::View | PMX::BoneFlags::Manipulable, (int)b->flags);
			Assert::AreEqual(15U, b->size.attachTo);

			b = m->bones[226];
			Assert::AreEqual(L"スカート_3_7IK", b->nameJP.c_str());
			Assert::AreEqual(-1.470095f, b->position.x, 0.0001f);
			Assert::AreEqual(9.860104f, b->position.y, 0.0001f);
			Assert::AreEqual(-1.274444f, b->position.z, 0.0001f);
			Assert::AreEqual(4U, b->parent);
			Assert::AreEqual(0, b->deformation);
			Assert::AreEqual(PMX::BoneFlags::Rotatable | PMX::BoneFlags::View | PMX::BoneFlags::Manipulable | PMX::BoneFlags::IK | PMX::BoneFlags::LocalAxis, (int)b->flags);
			Assert::AreEqual(0.0f, b->size.length.x, 0.0001f);
			Assert::AreEqual(-0.4f, b->size.length.y, 0.0001f);
			Assert::AreEqual(0.0f, b->size.length.z, 0.0001f);
			Assert::AreEqual(218U, b->ik.target);
			Assert::AreEqual(10, b->ik.count);
#define PI 3.14159265f
			Assert::AreEqual(57.29578f * PI / 180.0f, b->ik.angleLimit, 0.0001f); // rad to deg
			PMX::IKnode *n = &b->ik.links[1];
			Assert::AreEqual(202U, n->bone);
			Assert::IsFalse(n->limitAngle);
		}

		TEST_METHOD(FrameTest)
		{
			Model *m = createModel();

		}

		TEST_METHOD(RigidBodyTest)
		{
			Model *m = createModel();

		}

		TEST_METHOD(JointsTest) {
			Model *m = createModel();
		}

		TEST_METHOD(ReadAllTest)
		{
			Model *m = createModel();

			struct _stat64 s;
			_wstat64(L"../Debug/AppData/BeatMap/Models/Tda_luka_NAL76HN06/luka_Ln76_n19_06.pmx", &s);

			Assert::AreEqual((uint64_t)s.st_size, m->lastpos);
		}

	};
}