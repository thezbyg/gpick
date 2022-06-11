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

#ifndef GPICK_MATH_RECTANGLE_H_
#define GPICK_MATH_RECTANGLE_H_
#include "Vector.h"
namespace math {
template<typename T>
struct Rectangle {
	Rectangle() {
		m_empty = true;
	};
	Rectangle(const T &x1, const T &y1, const T &x2, const T &y2):
		m_x1(x1),
		m_y1(y1),
		m_x2(x2),
		m_y2(y2) {
		m_empty = false;
	};
	const Rectangle operator=(const Rectangle &rect) {
		m_x1 = rect.m_x1;
		m_y1 = rect.m_y1;
		m_x2 = rect.m_x2;
		m_y2 = rect.m_y2;
		m_empty = rect.m_empty;
		return *this;
	};
	const Rectangle operator+(const Rectangle &rect) const {
		if (rect.m_empty)
			return *this;
		if (m_empty)
			return rect;
		Rectangle r;
		if (m_x1 < rect.m_x1)
			r.m_x1 = m_x1;
		else
			r.m_x1 = rect.m_x1;
		if (m_y1 < rect.m_y1)
			r.m_y1 = m_y1;
		else
			r.m_y1 = rect.m_y1;
		if (m_x2 > rect.m_x2)
			r.m_x2 = m_x2;
		else
			r.m_x2 = rect.m_x2;
		if (m_y2 > rect.m_y2)
			r.m_y2 = m_y2;
		else
			r.m_y2 = rect.m_y2;
		return r;
	};
	const Rectangle operator+=(const Rectangle &rect) {
		*this = *this + rect;
		return *this;
	};
	Rectangle impose(const Rectangle &rect) const {
		Rectangle r;
		r.m_x1 = rect.m_x1 + m_x1 * rect.getWidth();
		r.m_y1 = rect.m_y1 + m_y1 * rect.getHeight();
		r.m_x2 = rect.m_x1 + m_x2 * rect.getWidth();
		r.m_y2 = rect.m_y1 + m_y2 * rect.getHeight();
		r.m_empty = false;
		return r;
	}
	bool isInside(const T &x, const T &y) const {
		if (x < m_x1 || x > m_x2 || y < m_y1 || y > m_y2)
			return false;
		else
			return true;
	}
	bool isInside(const Rectangle &r) const {
		if (m_empty || r.m_empty) return false;
		if (m_x1 < r.m_x1 || m_x2 > r.m_x2) return false;
		if (m_y1 < r.m_y1 || m_y2 > r.m_y2) return false;
		return true;
	}
	Rectangle positionInside(const Rectangle &rect) const {
		Rectangle r;
		r.m_empty = false;
		if (rect.m_x1 < m_x1) {
			r.m_x1 = m_x1;
			r.m_x2 = m_x1 + rect.getWidth();
		} else if (rect.m_x2 > m_x2) {
			r.m_x1 = m_x2 - rect.getWidth();
			r.m_x2 = m_x2;
		} else {
			r.m_x1 = rect.m_x1;
			r.m_x2 = rect.m_x2;
		}
		if (rect.m_y1 < m_y1) {
			r.m_y1 = m_y1;
			r.m_y2 = m_y1 + rect.getHeight();
		} else if (rect.m_y2 > m_y2) {
			r.m_y1 = m_y2 - rect.getHeight();
			r.m_y2 = m_y2;
		} else {
			r.m_y1 = rect.m_y1;
			r.m_y2 = rect.m_y2;
		}
		return r;
	}
	bool isEmpty() const {
		return m_empty;
	};
	const T &getX() const {
		return m_x1;
	};
	const T &getY() const {
		return m_y1;
	};
	T getWidth() const {
		return m_x2 - m_x1;
	};
	T getHeight() const {
		return m_y2 - m_y1;
	};
	T getCenterX() const {
		return (m_x1 + m_x2) / 2;
	};
	T getCenterY() const {
		return (m_y1 + m_y2) / 2;
	};
	const T &getLeft() const {
		return m_x1;
	};
	const T &getTop() const {
		return m_y1;
	};
	const T &getRight() const {
		return m_x2;
	};
	const T &getBottom() const {
		return m_y2;
	};
	Vector<T, 2> position() const {
		return Vector<T, 2>(m_x1, m_y1);
	}
	Vector<T, 2> center() const {
		return Vector<T, 2>((m_x1 + m_x2) / 2, (m_y1 + m_y2) / 2);
	}
	Vector<T, 2> size() const {
		return Vector<T, 2>(m_x2 - m_x1, m_y2 - m_y1);
	}
private:
	bool m_empty;
	T m_x1, m_y1, m_x2, m_y2;
};
using Rectanglef = Rectangle<float>;
using Rectangled = Rectangle<double>;
using Rectanglei = Rectangle<int>;
}
#endif /* GPICK_MATH_RECTANGLE_H_ */
