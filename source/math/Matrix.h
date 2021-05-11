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

#ifndef GPICK_MATH_MATRIX_H_
#define GPICK_MATH_MATRIX_H_
#include "Vector.h"
#include <algorithm>
#include <initializer_list>
#include <type_traits>
#include <stdexcept>
#include <optional>
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
			for (unsigned int row = 0; row < N; ++row) {
				for (unsigned int column = 0; column < N; ++column) {
					data[row][column] = column == row ? 1 : 0;
				}
			}
		} else if (initialize == Initialize::zero) {
			for (unsigned int i = 0; i < N * N; ++i) {
				flatData[i] = 0;
			}
		}
	}
	CommonMatrix(const CommonMatrix &matrix) {
		for (unsigned int i = 0; i < N * N; ++i) {
			flatData[i] = matrix.flatData[i];
		}
	}
	CommonMatrix &operator=(const CommonMatrix &matrix) {
		for (unsigned int i = 0; i < N * N; ++i) {
			flatData[i] = matrix.flatData[i];
		}
		return *this;
	}
	CommonMatrix(const CommonMatrix<T, N + 1, Matrix> &matrix, unsigned int skipColumn, unsigned int skipRow) {
		for (unsigned int column = 0; column < N; ++column) {
			for (unsigned int row = 0; row < N; ++row) {
				data[row][column] = matrix.data[row >= skipRow ? row + 1 : row][column >= skipColumn ? column + 1 : column];
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
		for (unsigned int column = 0; column < N; ++column) {
			double t = 1;
			for (unsigned int row = 0; row < N; ++row) {
				t *= data[row][(column + row) % N];
			}
			result += t;
		}
		for (unsigned int column = 0; column < N; ++column) {
			double t = 1;
			for (unsigned int row = 0; row < N; ++row) {
				t *= data[row][(column + N - row - 1) % N];
			}
			result -= t;
		}
		return result;
	};
	std::optional<Matrix> inverse() const {
		auto determinant = this->determinant();
		if (determinant == 0)
			return std::optional<Matrix>();
		double determinantInverse = 1.0 / determinant;
		Matrix result;
		for (unsigned int column = 0; column < N; ++column) {
			for (unsigned int row = 0; row < N; ++row) {
				if (((column + row) & 1) == 0)
					result.data[column][row] = CommonMatrix<T, N - 1, Matrix>(*this, column, row).determinant() * determinantInverse;
				else
					result.data[column][row] = -CommonMatrix<T, N - 1, Matrix>(*this, column, row).determinant() * determinantInverse;
			}
		}
		return result;
	};
	Matrix transpose() const {
		Matrix result(Initialize::none);
		for (unsigned int column = 0; column < N; ++column) {
			for (unsigned int row = 0; row < N; ++row) {
				result.data[row][column] = data[column][row];
			}
		}
		return result;
	};
	Matrix operator*(const Matrix &matrix) const {
		Matrix result(Initialize::zero);
		for (unsigned int column = 0; column < N; ++column) {
			for (unsigned int row = 0; row < N; ++row) {
				for (unsigned int k = 0; k < N; ++k) {
					result.data[row][column] += data[k][column] * matrix.data[row][k];
				}
			}
		}
		return result;
	};
	Matrix operator+(const Matrix &matrix) const {
		Matrix result(Initialize::zero);
		for (unsigned int i = 0; i < N * N; ++i) {
			result.flatData[i] = flatData[i] +  matrix.flatData[i];
		}
		return result;
	};
	Matrix operator-(const Matrix &matrix) const {
		Matrix result(Initialize::zero);
		for (unsigned int i = 0; i < N * N; ++i) {
			result.flatData[i] = flatData[i] - matrix.flatData[i];
		}
		return result;
	};
	template<typename Value>
	Matrix operator*(const Value value) const {
		Matrix result(Initialize::zero);
		for (unsigned int i = 0; i < N * N; ++i) {
			result.flatData[i] = flatData[i] * value;
		}
		return result;
	};
	template<typename Value>
	Matrix operator/(const Value value) const {
		Matrix result(Initialize::zero);
		for (unsigned int i = 0; i < N * N; ++i) {
			result.flatData[i] = flatData[i] / value;
		}
		return result;
	};
	const T operator[](size_t index) const {
		if (index >= N * N)
			throw std::invalid_argument("index");
		return flatData[index];
	};
	T &operator[](size_t index) {
		if (index >= N * N)
			throw std::invalid_argument("index");
		return flatData[index];
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
	for (unsigned int column = 0; column < N; ++column) {
		T value = 0;
		for (unsigned int row = 0; row < N; ++row) {
			value += vector.data[row] * matrix.data[row][column];
		}
		result.data[column] = value;
	}
	return result;
};
template<typename T, unsigned int N>
Vector<T, N> operator*(const Matrix<T, N> &matrix, const Vector<T, N> &vector) {
	Vector<T, N> result;
	for (unsigned int row = 0; row < N; ++row) {
		T value = 0;
		for (unsigned int column = 0; column < N; ++column) {
			value += vector.data[column] * matrix.data[row][column];
		}
		result.data[row] = value;
	}
	return result;
};
}
#endif /* GPICK_MATH_MATRIX_H_ */
