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

#ifndef GPICK_MATH_BEZIER_CUBIC_CURVE_H_
#define GPICK_MATH_BEZIER_CUBIC_CURVE_H_
#include <stdexcept>
namespace math {
template<typename Point, typename T>
struct BezierCubicCurve {
	BezierCubicCurve(const Point &p0, const Point &p1, const Point &p2, const Point &p3):
		m_points { p0, p1, p2, p3 } {
	};
	Point operator()(const T &t) {
		T t2 = 1 - t;
		return m_points[0] * (t2 * t2 * t2) + m_points[1] * (3 * (t2 * t2) * t) + m_points[2] * (3 * t2 * t * t) + m_points[3] * (t * t * t);
	};
	BezierCubicCurve &operator=(const BezierCubicCurve &curve) {
		for (size_t i = 0; i < sizeof(m_points) / sizeof(Point); i++)
			m_points[i] = curve.m_points[i];
		return *this;
	};
	const Point &operator[](size_t index) const {
		if (index >= sizeof(m_points) / sizeof(Point))
			throw std::invalid_argument("index");
		return m_points[index];
	}
private:
	Point m_points[4];
};
}
#endif /* GPICK_MATH_BEZIER_CUBIC_CURVE_H_ */
