#include "stdafx.h"
#include "CppUnitTest.h"
#include "../XBeat/PMX/PMXModel.h"
#include "../XBeat/PMX/PMXBone.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace PMX;

namespace PMXTest
{
	TEST_CLASS(BoneTest)
	{
	private:
		static Model* loadModel(std::shared_ptr<Physics::Environment> physics)
		{
			std::wstring file = L"../XBeat/Data/Models/2013RacingMikuMMD/2013RacingMiku_2.1.pmx";
			Model *model = new Model();
			if (!model->LoadModel(file) || !model->Initialize(nullptr, physics))
				throw std::exception("Failed to load the model");

			return model;
		}
		static std::shared_ptr<Physics::Environment> createPhysics()
		{
			std::shared_ptr<Physics::Environment> physics(new Physics::Environment);
			physics->initialize();
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

		TEST_METHOD(TestIK)
		{
		}

	};
}