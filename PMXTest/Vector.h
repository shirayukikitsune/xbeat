#pragma once

#ifdef _MSC_VER
#include <xmmintrin.h>
#endif

enum Alignment {
	Aligned,
	Unaligned
};

class Vector
{
public:
	Vector() {
		v = _mm_set1_ps(0.0f);
		dirty = true;
	}
	Vector(float x, float y, float z, float w) {
		fv[0] = x;
		fv[1] = y;
		fv[2] = z;
		fv[3] = w;
		this->v = _mm_loadu_ps(fv);
		dirty = false;
	}
	Vector(float *v) {
		memcpy(fv, v, sizeof (float) * 4);
		this->v = _mm_loadu_ps(v);
		dirty = false;
	}
	Vector(__m128 _v) : v(_v) { dirty = true; }

	inline float* getV() {
		if (dirty) {
			_mm_storeu_ps(fv, v);
			dirty = false;
		}

		return fv;
	}
	inline float getX() {
		return getV()[0];
	}
	inline float getY() {
		return getV()[1];
	}
	inline float getZ() {
		return getV()[2];
	}
	inline float getW() {
		return getV()[3];
	}

private:
	inline static __m128 _dotProduct(const __m128 &a, const __m128 &b)
	{
#if USE_SSE == 4
		return _mm_dp_ps(a, b, 0xFF);
#elif USE_SSE == 3
		__m128 out = _mm_mul_ps(a, b);

		out = _mm_hadd_ps(out, out);
		out = _mm_hadd_ps(out, out);

		return out;
#else
		__m128 mult = _mm_mul_ps(a, b);
		__m128 shuf1 = _mm_shuffle_ps(mult, mult, _MM_SHUFFLE(0, 3, 2, 1));
		__m128 shuf2 = _mm_shuffle_ps(mult, mult, _MM_SHUFFLE(1, 0, 3, 2));
		__m128 shuf3 = _mm_shuffle_ps(mult, mult, _MM_SHUFFLE(2, 1, 0, 3));

		return _mm_add_ss(_mm_add_ss(_mm_add_ss(mult, shuf1), shuf2), shuf3);
#endif
	}

	__m128 v;
	float fv[4];
	bool dirty;
public:
#pragma region Type Conversion
	inline operator __m128 () const {
		return this->v;
	}
	inline operator __m128 () {
		return this->v;
	}
#pragma endregion

#pragma region Assignment
	inline Vector operator=(const __m128 &other) {
		this->v = other;
		dirty = true;

		return *this;
	}
	inline Vector operator=(const Vector &other) {
		this->v = other.v;
		memcpy(this->fv, other.fv, sizeof (fv));
		this->dirty = other.dirty;

		return *this;
	}
#pragma endregion 

#pragma region Vector operators
	inline Vector operator+(const Vector &other) {
		return _mm_add_ps(*this, other);
	}

	inline Vector operator-(const Vector &other) {
		return _mm_sub_ps(*this, other);
	}

	inline float operator*(const Vector &other) {
		return this->Scalar(other);
	}
#pragma endregion

#pragma region Float operators
	inline Vector operator/(const float &other) const {
		return _mm_div_ps(*this, _mm_set1_ps(other));
	}

	inline Vector operator*(const float &other) const {
		return _mm_mul_ps(*this, _mm_set1_ps(other));
	}

	inline Vector operator+(const float &other) {
		return _mm_add_ps(*this, _mm_set1_ps(other));
	}

	inline Vector operator-(const float &other) {
		return _mm_sub_ps(*this, _mm_set1_ps(other));
	}
#pragma endregion

	inline float Scalar(const Vector &other) {
		return _mm_cvtss_f32(_dotProduct(*this, other));
	}

	inline float LengthSquared() {
		return this->Scalar(*this);
	}

	inline float Length() {
		return _mm_cvtss_f32(_mm_sqrt_ss(_dotProduct(*this, *this)));
	}

	inline Vector Normalize() {
		return _mm_mul_ps(*this, _mm_rsqrt_ps(_dotProduct(*this, *this)));;
	}

	inline Vector Cross(const Vector &other) {
		return _mm_sub_ps(
				_mm_mul_ps(_mm_shuffle_ps(*this, *this, _MM_SHUFFLE(3, 0, 2, 1)), _mm_shuffle_ps(other, other, _MM_SHUFFLE(3, 1, 0, 2))),
				_mm_mul_ps(_mm_shuffle_ps(*this, *this, _MM_SHUFFLE(3, 1, 0, 2)), _mm_shuffle_ps(other, other, _MM_SHUFFLE(3, 0, 2, 1)))
			);
	}
};

