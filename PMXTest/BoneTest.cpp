#include "stdafx.h"
#include "CppUnitTest.h"
#include "../XBeat/Renderer/PMX/PMXModel.h"
#include "../XBeat/Renderer/PMX/PMXBone.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace Renderer::PMX;

namespace PMXTest
{
	TEST_CLASS(BoneTest)
	{
	private:
		static Model* loadModel(std::shared_ptr<Physics::Environment> physics)
		{
			std::wstring file = L"../XBeat/Data/Models/銀獅式波音リツ_レクイエム_ver1.20/銀獅式波音リツ_レクイエム_ver1.20.pmx";
			Model *model = new Model();
			if (!model->Initialize(nullptr, file, physics, nullptr))
				throw std::exception("Failed to load the model");

			return model;
		}
		static std::shared_ptr<Physics::Environment> createPhysics()
		{
			std::shared_ptr<Physics::Environment> physics(new Physics::Environment);
			physics->Initialize(nullptr);
			return physics;
		}
	public:
		
		TEST_METHOD(TestTranslation)
		{
			auto physics = createPhysics();
			auto model = loadModel(physics);

			auto bone = model->GetBoneByName(L"まゆげ（右）");
			bone->Translate(btVector3(10.0f, 0.0f, 0.0f));
			
			auto position = bone->GetPosition();
			Assert::AreEqual(9.49f, position.x(), 0.01f);
			Assert::AreEqual(18.34f, position.y(), 0.01f);
			Assert::AreEqual(-1.12f, position.z(), 0.01f);

			bone->Translate(btVector3(0.0f, 10.0f, 0.0f));

			position = bone->GetPosition();
			Assert::AreEqual(9.49f, position.x(), 0.01f);
			Assert::AreEqual(28.34f, position.y(), 0.01f);
			Assert::AreEqual(-1.12f, position.z(), 0.01f);
		}

		TEST_METHOD(TestVertexTranslation)
		{
			// This method will test the translation of a vertex affected by a bone
			auto physics = createPhysics();
			auto model = loadModel(physics);

			auto bone = model->GetBoneByName(L"まゆげ（右）");

			auto position = model->vertices[0]->GetFinalPosition();
			Assert::AreEqual(-0.83f, position.x(), 0.01f);
			Assert::AreEqual(18.30f, position.y(), 0.01f);
			Assert::AreEqual(-0.90f, position.z(), 0.01f);

			bone->Translate(btVector3(10.0f, 0.0f, 0.0f));
			position = model->vertices[0]->GetFinalPosition();

			Assert::AreEqual(9.17f, position.x(), 0.01f);
			Assert::AreEqual(18.30f, position.y(), 0.01f);
			Assert::AreEqual(-0.90f, position.z(), 0.01f);

			bone->Translate(btVector3(0.0f, 10.0f, 0.0f));
			position = model->vertices[0]->GetFinalPosition();

			Assert::AreEqual(9.17f, position.x(), 0.01f);
			Assert::AreEqual(28.30f, position.y(), 0.01f);
			Assert::AreEqual(-0.90f, position.z(), 0.01f);
		}

	};
}