#pragma once

// Include all commonly used Direct3D headers here
#include <D3D11.h>
#include <DirectXMath.h>

namespace Renderer {
	// Type to be used by every DirectX interface, but should not be used for arrays
	template <class T>
	class DXType {
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
			if (ptr != nullptr) ptr->Release();

			ptr = other;
			if (other != nullptr) other->AddRef();
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
			if (ptr != nullptr) {
				ptr->Release();
				ptr = nullptr;
			}
		}
		DXType() {
			ptr = nullptr;
		}
		DXType(T* p) {
			ptr = p;
			ptr->AddRef();
		}
		DXType(const DXType<T> &other) {
			ptr = other.ptr;
			ptr->AddRef();
		}
	};
}
