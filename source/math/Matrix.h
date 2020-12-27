/*
 * Copyright (c) 2009-2020, Albertas Vyšniauskas
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

#ifndef GPICK_MATH_MATRIX_H_
#define GPICK_MATH_MATRIX_H_
#include "Vector.h"
#include <algorithm>
#include <initializer_list>
#include <type_traits>
#include <stdexcept>
#include <boost/optional.hpp>
namespace math {
namespace impl {
template<typename T, unsigned int N, typename Matrix>
struct CommonMatrix {
	static const unsigned int Size = N;
	enum class Initialize {
		identity,
		zero,
		none,
	};
	CommonMatrix(Initialize initialize) {
		if (initialize == Initialize::identity) {
			for (unsigned int i = 0; i < N; ++i) {
				for (unsigned int j = 0; j < N; ++j) {
					data[j][i] = i == j ? 1 : 0;
				}
			}
		} else if (initialize == Initialize::zero) {
			for (unsigned int i = 0; i < N; ++i) {
				for (unsigned int j = 0; j < N; ++j) {
					data[j][i] = 0;
				}
			}
		}
	}
	CommonMatrix(const CommonMatrix &matrix) {
		for (unsigned int i = 0; i < N; ++i) {
			for (unsigned int j = 0; j < N; ++j) {
				data[j][i] = matrix.data[j][i];
			}
		}
	}
	CommonMatrix &operator=(const CommonMatrix &matrix) {
		for (unsigned int i = 0; i < N * N; ++i) {
			flatData[i] = matrix.flatData[i];
		}
		return *this;
	}
	CommonMatrix(const CommonMatrix<T, N + 1, Matrix> &matrix, unsigned int skipI, unsigned int skipJ) {
		for (unsigned int i = 0; i < N; ++i) {
			for (unsigned int j = 0; j < N; ++j) {
				data[j][i] = matrix.data[j >= skipJ ? j + 1 : j][i >= skipI ? i + 1 : i];
			}
		}
	}
	bool operator==(const CommonMatrix &matrix) const {
		for (unsigned int i = 0; i < N * N; ++i) {
			if (flatData[i] != matrix.flatData[i])
				return false;
		}
		return true;
	}
	template<unsigned int X = N, std::enable_if_t<(X == 1), int> = 0>
	auto determinant() const {
		return data[0][0];
	}
	template<unsigned int X = N, std::enable_if_t<(X == 2), int> = 0>
	auto determinant() const {
		return data[0][0] * data[1][1] - data[1][0] * data[0][1];
	}
	template<unsigned int X = N, std::enable_if_t<(X >= 3), int> = 0>
	auto determinant() const {
		double result = 0;
		for (unsigned int i = 0; i < N; ++i) {
			double t = 1;
			for (unsigned int j = 0; j < N; ++j) {
				t *= data[j][(i + j) % N];
			}
			result += t;
		}
		for (unsigned int i = 0; i < N; ++i) {
			double t = 1;
			for (unsigned int j = 0; j < N; ++j) {
				t *= data[j][(i + N - j - 1) % N];
			}
			result -= t;
		}
		return result;
	};
	boost::optional<Matrix> inverse() const {
		auto determinant = this->determinant();
		if (determinant == 0)
			return boost::optional<Matrix>();
		double determinantInverse = 1.0 / determinant;
		Matrix result;
		for (unsigned int i = 0; i < N; ++i) {
			for (unsigned int j = 0; j < N; ++j) {
				if (((i + j) & 1) == 0)
					result.data[i][j] = CommonMatrix<T, N - 1, Matrix>(*this, i, j).determinant() * determinantInverse;
				else
					result.data[i][j] = -CommonMatrix<T, N - 1, Matrix>(*this, i, j).determinant() * determinantInverse;
			}
		}
		return result;
	};
	Matrix transpose() const {
		Matrix result(Initialize::none);
		for (unsigned int i = 0; i < N; ++i) {
			for (unsigned int j = 0; j < N; ++j) {
				result.data[j][i] = data[i][j];
			}
		}
		return result;
	};
	Matrix operator*(const Matrix &matrix) const {
		Matrix result(Initialize::zero);
		for (unsigned int i = 0; i < N; ++i) {
			for (unsigned int j = 0; j < N; ++j) {
				for (unsigned int k = 0; k < N; ++k) {
					result.data[j][i] += data[k][i] * matrix.data[j][k];
				}
			}
		}
		return result;
	};
	const T operator[](size_t index) const {
		if (index >= N * N)
			throw std::invalid_argument("index");
		return data[index / N][index % N];
	};
	T &operator[](size_t index) {
		if (index >= N * N)
			throw std::invalid_argument("index");
		return data[index / N][index % N];
	};
	union {
		T data[N][N];
		T flatData[N * N];
	};
protected:
	template<typename... Args, std::enable_if_t<sizeof...(Args) == N * N, int> = 0>
	CommonMatrix(Args... args) {
		std::copy_n(std::initializer_list<T>({ args... }).begin(), N * N, this->flatData);
	};
private:
	Matrix &asMatrix() {
		return *reinterpret_cast<Matrix *>(this);
	};
};
}
template<typename T, unsigned int N>
struct Matrix;
template<typename T>
struct Matrix<T, 3>: public impl::CommonMatrix<T, 3, Matrix<T, 3>> {
	using Base = impl::CommonMatrix<T, 3, Matrix<T, 3>>;
	using Initialize = typename Base::Initialize;
	Matrix():
		Base(Initialize::identity) {};
	Matrix(Initialize initialize):
		Base(initialize) {};
	Matrix(const Matrix &matrix):
		Base(matrix) {};
	template<typename... Args, std::enable_if_t<sizeof...(Args) == 9, int> = 0>
	Matrix(Args... args):
		Base(args...) {};
	Matrix &operator=(const Matrix &matrix) {
		Base::operator=(matrix);
		return *this;
	}
};
using Matrix3f = Matrix<float, 3>;
using Matrix3d = Matrix<double, 3>;
template<typename T, unsigned int N>
Vector<T, N> operator*(const Vector<T, N> &vector, const Matrix<T, N> &matrix) {
	Vector<T, N> result;
	for (unsigned int j = 0; j < N; ++j) {
		T value = 0;
		for (unsigned int i = 0; i < N; ++i) {
			value += vector.data[i] * matrix.data[j][i];
		}
		result.data[j] = value;
	}
	return result;
};
}
#endif /* GPICK_MATH_MATRIX_H_ */
