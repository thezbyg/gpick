/*
 * Copyright (c) 2009-2022, Albertas Vyšniauskas
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

#ifndef GPICK_COMMON_FIRST_H_
#define GPICK_COMMON_FIRST_H_
#include "TypeTraits.h"
#include <tuple>
#include <utility>
namespace common {
template<typename ValueT, typename OpT = std::less<ValueT>, typename... Args> struct First {
	First():
		m_empty(true) {
	}
	First(const ValueT &value, Args... params):
		m_empty(false),
		m_value(value),
		m_data(std::forward_as_tuple(params...)) {
	}
	First(ValueT &&value, Args... params):
		m_empty(false),
		m_value(std::move(value)),
		m_data(std::forward_as_tuple(params...)) {
	}
	First(const First &) = delete;
	First(First &&first):
		m_empty(first.m_empty),
		m_value(std::move(first.m_value)),
		m_data(std::move(first.m_data)) {
	}
	First &operator=(const First &) = delete;
	First &operator=(First &&first) {
		m_empty = first.m_empty;
		m_value = std::move(first.m_value);
		m_data = std::move(first.m_data);
		return *this;
	}
	~First() {
	}
	operator bool() const {
		return !m_empty;
	}
	const ValueT &value() const {
		return m_value;
	}
	ValueT &value() {
		return m_value;
	}
	First &operator()(const ValueT &value, Args... params) {
		if (m_empty) {
			m_value = value;
			m_data = std::move(std::forward_as_tuple(params...));
			m_empty = false;
		} else if (OpT{}(value, m_value)) {
			m_value = value;
			m_data = std::move(std::forward_as_tuple(params...));
		}
		return *this;
	}
	template<typename T>
	auto data() {
		return std::get<T>(m_data);
	}
	template<typename T>
	auto data() const {
		return std::get<T>(m_data);
	}
	void reset() {
		m_empty = true;
	}
private:
	bool m_empty;
	ValueT m_value;
	std::tuple<detail::UnwrapAndDecay<Args>...> m_data;
};
}
#endif /* GPICK_COMMON_FIRST_H_ */
