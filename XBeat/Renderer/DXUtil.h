#pragma once

// Include all commonly used Direct3D headers here
#include <D3D11.h>
#include <DirectXMath.h>

namespace Renderer {
	// Type to be used by every DirectX interface, but should not be used for arrays
	template <class T>
	class DXType {
	public:
		typedef T* pointer;
		typedef T const * const_pointer;

	private:
		T* ptr;

	public:
		T*const* operator&() const {
			return &ptr;
		}
		T** operator&() {
			return &ptr;
		}
		const T* operator->() const {
			return ptr;
		}
		T* operator->() {
			return ptr;
		}
		operator T*() {
			return ptr;
		}
		void operator=(T* other) {
			reset();
			ptr = other;
			if (other != nullptr) other->AddRef();
		}
		void operator=(DXType<T> &&other) {
			reset();
			ptr = std::move(other.ptr);
		}
		operator bool() {
			return ptr != nullptr;
		}

		void reset() {
			if (ptr != nullptr) {
				ptr->Release();
				ptr = nullptr;
			}
		}

		~DXType() {
			reset();
		}
		DXType() {
			ptr = nullptr;
		}
		DXType(T* p) {
			ptr = p;
			if (p != nullptr)
				ptr->AddRef();
		}
		DXType(const DXType<T> &other) {
			ptr = other.ptr;
			if (ptr != nullptr)
				ptr->AddRef();
		}
		DXType(DXType<T> &&other)
		: ptr(std::move(other.ptr)) {
		}
	};
}
