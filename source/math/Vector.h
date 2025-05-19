/*
 * Copyright (c) 2009-2021, Albertas Vyšniauskas
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of the software author nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef GPICK_MATH_VECTOR_H_
#define GPICK_MATH_VECTOR_H_
#include "Algorithms.h"
#include <algorithm>
#include <initializer_list>
#include <type_traits>
#include <stdexcept>
#include <cmath>
namespace math {
namespace impl {
template<typename T, unsigned int N>
struct VectorBase;
template<typename T>
struct VectorBase<T, 2> {
	static const unsigned int Size = 2;
	union {
#pragma pack(push, 1)
		struct {
			T x, y;
		};
		T data[2];
#pragma pack(pop)
	};
	VectorBase():
		x(0), y(0) {};
	VectorBase(const T &x, const T &y):
		x(x), y(y) {};
	VectorBase(const VectorBase &value):
		x(value.x), y(value.y) {};
};
template<typename T>
struct VectorBase<T, 3> {
	static const unsigned int Size = 3;
	union {
#pragma pack(push, 1)
		struct {
			T x, y, z;
		};
		struct {
			T r, g, b;
		};
		T data[3];
#pragma pack(pop)
	};
	VectorBase():
		x(0), y(0), z(0) {};
	VectorBase(const T &x, const T &y, const T &z):
		x(x), y(y), z(z) {};
	VectorBase(const VectorBase &value):
		x(value.x), y(value.y), z(value.z) {};
};
template<typename T, unsigned int N, typename Vector>
struct CommonVector: public VectorBase<T, N> {
	CommonVector():
		VectorBase<T, N>() {
		for (unsigned int i = 0; i < N; i++) {
			this->data[i] = 0;
		}
	};
	CommonVector(const CommonVector &vector):
		VectorBase<T, N>() {
		for (unsigned int i = 0; i < N; i++) {
			this->data[i] = vector.data[i];
		}
	};
	template<typename Value>
	Vector operator*(const Value value) const {
		Vector result;
		for (unsigned int i = 0; i < N; i++) {
			result.data[i] = this->data[i] * value;
		}
		return result;
	};
	template<typename Value>
	Vector operator/(const Value value) const {
		Vector result;
		for (unsigned int i = 0; i < N; i++) {
			result.data[i] = this->data[i] / value;
		}
		return result;
	};
	template<typename Value>
	Vector operator+(const Value value) const {
		Vector result;
		for (unsigned int i = 0; i < N; i++) {
			result.data[i] = this->data[i] + value;
		}
		return result;
	};
	template<typename Value>
	Vector operator-(const Value value) const {
		Vector result;
		for (unsigned int i = 0; i < N; i++) {
			result.data[i] = this->data[i] - value;
		}
		return result;
	};
	template<typename Value>
	Vector &operator*=(const Value value) {
		for (unsigned int i = 0; i < N; i++) {
			this->data[i] *= value;
		}
		return asVector();
	};
	template<typename Value>
	Vector &operator/=(const Value value) {
		for (unsigned int i = 0; i < N; i++) {
			this->data[i] /= value;
		}
		return asVector();
	};
	template<typename Value>
	Vector &operator+=(const Value value) {
		for (unsigned int i = 0; i < N; i++) {
			this->data[i] += value;
		}
		return asVector();
	};
	template<typename Value>
	Vector &operator-=(const Value value) {
		for (unsigned int i = 0; i < N; i++) {
			this->data[i] -= value;
		}
		return asVector();
	};
	Vector operator-() const {
		Vector result;
		for (unsigned int i = 0; i < N; i++) {
			result.data[i] = -this->data[i];
		}
		return result;
	};
	Vector &operator=(const Vector &vector) {
		for (unsigned int i = 0; i < N; i++) {
			this->data[i] = vector.data[i];
		}
		return asVector();
	};
	Vector &operator*=(const Vector &vector) {
		for (unsigned int i = 0; i < N; i++) {
			this->data[i] *= vector.data[i];
		}
		return asVector();
	};
	Vector operator*(const Vector &vector) const {
		Vector result;
		for (unsigned int i = 0; i < N; i++) {
			result.data[i] = this->data[i] * vector.data[i];
		}
		return result;
	};
	Vector &operator/=(const Vector &vector) {
		for (unsigned int i = 0; i < N; i++) {
			this->data[i] /= vector.data[i];
		}
		return asVector();
	};
	Vector operator/(const Vector &vector) const {
		Vector result;
		for (unsigned int i = 0; i < N; i++) {
			result.data[i] = this->data[i] / vector.data[i];
		}
		return result;
	};
	Vector operator+(const Vector &vector) const {
		Vector result;
		for (unsigned int i = 0; i < N; i++) {
			result.data[i] = this->data[i] + vector.data[i];
		}
		return result;
	};
	Vector operator-(const Vector &vector) const {
		Vector result;
		for (unsigned int i = 0; i < N; i++) {
			result.data[i] = this->data[i] - vector.data[i];
		}
		return result;
	};
	T &operator[](size_t index) {
		if (index >= N)
			throw std::invalid_argument("index");
		return this->data[index];
	};
	const T operator[](size_t index) const {
		if (index >= N)
			throw std::invalid_argument("index");
		return this->data[index];
	};
	bool operator==(const Vector &vector) const {
		for (unsigned int i = 0; i < N; i++) {
			if (this->data[i] != vector.data[i])
				return false;
		}
		return true;
	};
	bool operator!=(const Vector &vector) const {
		for (unsigned int i = 0; i < N; i++) {
			if (this->data[i] != vector.data[i])
				return true;
		}
		return false;
	};
	auto squaredLength() const {
		double result = 0;
		for (unsigned int i = 0; i < N; i++) {
			result += this->data[i] * this->data[i];
		}
		return result;
	};
	auto length() const {
		return std::sqrt(squaredLength());
	};
	template<typename Length>
	Length length() const {
		return static_cast<Length>(std::sqrt(squaredLength()));
	}
	Vector &normalize() {
		auto vectorLength = length();
		for (unsigned int i = 0; i < N; i++) {
			this->data[i] = static_cast<T>(this->data[i] / vectorLength);
		}
		return asVector();
	};
	Vector &clamp(T min, T max) {
		for (unsigned int i = 0; i < N; i++) {
			this->data[i] = math::clamp(this->data[i], min, max);
		}
		return asVector();
	};
	Vector normalizeCopy() const {
		return Vector(*this).normalize();
	};
	Vector clampCopy(T min, T max) const {
		return Vector(*this).clamp();
	};
	T dotProduct(const Vector &vector) const {
		T result {};
		for (unsigned int i = 0; i < N; i++) {
			result += this->data[i] * vector.data[i];
		}
		return result;
	};
	template<typename Modifier>
	Vector &modifyValues(Modifier modifier) {
		for (unsigned int i = 0; i < N; i++) {
			this->data[i] = modifier(this->data[i]);
		}
		return asVector();
	};
protected:
	template<typename... Args, std::enable_if_t<sizeof...(Args) == N, int> = 0>
	CommonVector(Args... args) {
		std::copy_n(std::initializer_list<T>({ args... }).begin(), N, this->data);
	};
private:
	Vector &asVector() {
		return *reinterpret_cast<Vector *>(this);
	};
};
}
/** \struct Vector
 * \brief N dimensional vector of type T
 */
template<typename T, unsigned int N>
struct Vector;
template<typename T>
struct Vector<T, 2>: public impl::CommonVector<T, 2, Vector<T, 2>> {
	using Base = impl::CommonVector<T, 2, Vector<T, 2>>;
	Vector():
		Base() {};
	Vector(const T &x, const T &y):
		Base(x, y) {};
	Vector(const Vector &vector):
		Base(vector) {}
};
template<typename T>
struct Vector<T, 3>: public impl::CommonVector<T, 3, Vector<T, 3>> {
	using Base = impl::CommonVector<T, 3, Vector<T, 3>>;
	Vector():
		Base() {};
	Vector(const T &x, const T &y, const T &z):
		Base(x, y, z) {};
	Vector(const Vector &vector):
		Base(vector) {}
};
using Vector2f = Vector<float, 2>;
using Vector2d = Vector<double, 2>;
using Vector2i = Vector<int, 2>;
using Vector3f = Vector<float, 3>;
using Vector3d = Vector<double, 3>;
template<typename T, typename TFrom, unsigned int N>
Vector<T, N> vectorCast(const Vector<TFrom, N> &vector) {
	Vector<T, N> result;
	for (unsigned int i = 0; i < N; i++) {
		result[i] = static_cast<T>(vector[i]);
	}
	return result;
}
}
#endif /* GPICK_MATH_VECTOR_H_ */
