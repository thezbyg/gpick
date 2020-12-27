/*
 * Copyright (c) 2009-2019, Albertas Vyšniauskas
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

#ifndef GPICK_MATH_ALGORITHMS_H_
#define GPICK_MATH_ALGORITHMS_H_
#include <tuple>
#include <cmath>
namespace math {
const double PI = 3.14159265359;
template<typename T>
inline T abs(T value) {
	return value < 0 ? -value : value;
}
template<typename T>
inline T clamp(T value, T min, T max) {
	return value < min ? min : (value < max ? value : max);
}
inline float clamp(float value, float min = 0.0f, float max = 1.0f) {
	return value < min ? min : (value < max ? value : max);
}
template<typename T, typename R>
inline T mix(T a, T b, R ratio) {
	return a * (1 - ratio) + b * ratio;
}
template<typename T>
inline T mix(T a, T b, float ratio = 0.5f) {
	return a * (1.0f - ratio) + b * ratio;
}
template<typename T>
inline T wrap(T value, T min, T max) {
	while (value < min)
		value += max - min;
	while (value >= max)
		value -= max - min;
	return value;
}
inline float wrap(float value) {
	return static_cast<float>(value - std::floor(value));
}
template<typename T>
inline T &min(T &val) {
	return val;
}
template<typename T0, typename T1, typename... Ts>
inline auto min(T0 &&val1, T1 &&val2, Ts &&... vs) {
	return (val2 > val1) ? min(val1, vs...) : min(val2, vs...);
}
template<typename T>
inline T &max(T &&val) {
	return val;
}
template<typename T0, typename T1, typename... Ts>
inline auto max(T0 &&val1, T1 &&val2, Ts &&... vs) {
	return (val2 < val1) ? max(val1, vs...) : max(val2, vs...);
}
template<typename T, typename... Ts>
inline auto minMax(T &&value, Ts &&... values) {
	return std::make_tuple<T &&, T &&>(min(value, values...), max(value, values...));
}
template<typename T>
inline auto minMax(T &&value1, T &&value2) {
	if (value2 < value1)
		return std::make_tuple<T &&, T &&>(value2, value1);
	else
		return std::make_tuple<T &&, T &&>(value1, value2);
}
template<typename T>
inline auto minMax(T &&value1, T &&value2, T &&value3) {
	if (value2 < value1) {
		if (value3 < value2) {
			return std::make_tuple<T &&, T &&>(value3, value1);
		} else if (value1 < value3) {
			return std::make_tuple<T &&, T &&>(value2, value3);
		} else {
			return std::make_tuple<T &&, T &&>(value2, value1);
		}
	} else {
		if (value3 < value1) {
			return std::make_tuple<T &&, T &&>(value3, value2);
		} else if (value2 < value3) {
			return std::make_tuple<T &&, T &&>(value1, value3);
		} else {
			return std::make_tuple<T &&, T &&>(value1, value2);
		}
	}
}
}
#endif /* GPICK_MATH_ALGORITHMS_H_ */
