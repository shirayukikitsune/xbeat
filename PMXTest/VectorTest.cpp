#include "stdafx.h"
#include "CppUnitTest.h"

#include "Vector.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace PMXTest {
	TEST_CLASS(VectorTest) {
	private:
		void init2Vectors(Vector &v1, Vector &v2) {
			v1 = Vector(1.0f, 2.0f, 3.0f, 4.0f);
			float v[4] = { 5.0f, 6.0f, 7.0f, 8.0f };
			v2 = Vector(v);
		}

	public:
		TEST_METHOD(VectorInitializeTest) {
			Vector v1, v2, v3;
			Assert::AreEqual(0.0f, v1.getX(), 0.00001f);
			Assert::AreEqual(0.0f, v1.getY(), 0.00001f);
			Assert::AreEqual(0.0f, v1.getZ(), 0.00001f);
			Assert::AreEqual(0.0f, v1.getW(), 0.00001f);

			init2Vectors(v2, v3);
			Assert::AreEqual(1.0f, v2.getX(), 0.00001f);
			Assert::AreEqual(2.0f, v2.getY(), 0.00001f);
			Assert::AreEqual(3.0f, v2.getZ(), 0.00001f);
			Assert::AreEqual(4.0f, v2.getW(), 0.00001f);

			Assert::AreEqual(5.0f, v3.getX(), 0.00001f);
			Assert::AreEqual(6.0f, v3.getY(), 0.00001f);
			Assert::AreEqual(7.0f, v3.getZ(), 0.00001f);
			Assert::AreEqual(8.0f, v3.getW(), 0.00001f);
		}

		TEST_METHOD(AddTest) {
			Vector v1, v2, v3;
			init2Vectors(v1, v2);

			v3 = v1 + v2;

			Assert::AreEqual(v1.getX() + v2.getX(), v3.getX(), 0.00001f);
			Assert::AreEqual(v1.getY() + v2.getY(), v3.getY(), 0.00001f);
			Assert::AreEqual(v1.getZ() + v2.getZ(), v3.getZ(), 0.00001f);
			Assert::AreEqual(v1.getW() + v2.getW(), v3.getW(), 0.00001f);
		}

		TEST_METHOD(SubTest) {
			Vector v1, v2, v3;
			init2Vectors(v1, v2);

			v3 = v1 - v2;

			Assert::AreEqual(v1.getX() - v2.getX(), v3.getX(), 0.00001f);
			Assert::AreEqual(v1.getY() - v2.getY(), v3.getY(), 0.00001f);
			Assert::AreEqual(v1.getZ() - v2.getZ(), v3.getZ(), 0.00001f);
			Assert::AreEqual(v1.getW() - v2.getW(), v3.getW(), 0.00001f);
		}

		TEST_METHOD(DotProduct) {
			Vector v1, v2;
			init2Vectors(v1, v2);

			Assert::AreEqual(70.0f, v1.Scalar(v2), 0.00001f);
			Assert::AreEqual(70.0f, v2.Scalar(v1), 0.00001f);
		}

		TEST_METHOD(CrossProduct) {
			Vector v1(1.0f, 0.0f, 0.0f, 0.0f), v2(0.0f, 1.0f, 0.0f, 0.0f), v3;

			v3 = v1.Cross(v2);

			Assert::AreEqual(0.0f, v3.getX());
			Assert::AreEqual(0.0f, v3.getY());
			Assert::AreEqual(1.0f, v3.getZ());
			Assert::AreEqual(0.0f, v3.getW());

			v3 = v2.Cross(v1);

			Assert::AreEqual(0.0f, v3.getX());
			Assert::AreEqual(0.0f, v3.getY());
			Assert::AreEqual(-1.0f, v3.getZ());
			Assert::AreEqual(0.0f, v3.getW());

			init2Vectors(v1, v2);

			v3 = v1.Cross(v2);
			Assert::AreEqual(-4.0f, v3.getX());
			Assert::AreEqual(8.0f, v3.getY());
			Assert::AreEqual(-4.0f, v3.getZ());
			Assert::AreEqual(0.0f, v3.getW());
		}

		TEST_METHOD(Length) {
			Vector v1(2.0f, 2.0f, 2.0f, 2.0f);

			Assert::AreEqual(16.0f, v1.LengthSquared());
			Assert::AreEqual(4.0f, v1.Length());
		}

		TEST_METHOD(Normalize) {
			Vector v1(2.0f, 2.0f, 2.0f, 2.0f);
			Vector n = v1.Normalize();
			
			Assert::AreEqual(0.5f, n.getX(), 0.0001f);
			Assert::AreEqual(0.5f, n.getY(), 0.0001f);
			Assert::AreEqual(0.5f, n.getZ(), 0.0001f);
			Assert::AreEqual(0.5f, n.getW(), 0.0001f);
			Assert::AreEqual(1.0f, n.LengthSquared(), 0.001f);
		}
	};
}