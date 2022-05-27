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

#ifndef GPICK_COMMON_SCOPED_H_
#define GPICK_COMMON_SCOPED_H_
#include "TypeTraits.h"
#include <tuple>
#include <utility>
namespace common {
namespace detail {
template<typename Function, typename Tuple, std::size_t... I>
auto apply(Function &f, Tuple &t, std::index_sequence<I...>) {
	return f(std::get<I>(t)...);
}
template<typename Function, typename Tuple>
auto apply(Function &f, Tuple &t) {
	static constexpr auto size = std::tuple_size<Tuple>::value;
	return apply(f, t, std::make_index_sequence<size> {});
}
}
template<typename Callable, typename... Args>
struct Scoped;
template<typename Callable>
struct Scoped<Callable> {
	Scoped(Callable callable):
		m_callable(callable),
		m_canceled(false) {
	}
	Scoped(Scoped &&scoped) {
		m_callable = std::move(scoped.m_callable);
		m_canceled = scoped.m_canceled;
		scoped.m_canceled = true;
	}
	Scoped(const Scoped &) = delete;
	Scoped &operator=(const Scoped &) = delete;
	~Scoped() {
		if constexpr (std::is_reference_v<Callable> || !std::is_convertible_v<Callable, bool>) {
			if (!m_canceled)
				m_callable();
		} else {
			if (!m_canceled && m_callable)
				m_callable();
		}
	}
	void cancel() {
		m_canceled = true;
	}
private:
	Callable m_callable;
	bool m_canceled;
};
template<typename Callable, typename... Args>
struct Scoped {
	Scoped(Callable callable, Args... params):
		m_callable(callable),
		m_arguments(std::forward_as_tuple(params...)),
		m_canceled(false) {
	}
	Scoped(Scoped &&scoped) {
		m_callable = std::move(scoped.m_callable);
		m_arguments = std::move(scoped.m_arguments);
		m_canceled = scoped.m_canceled;
		scoped.m_canceled = true;
	}
	Scoped(const Scoped &) = delete;
	Scoped &operator=(const Scoped &) = delete;
	~Scoped() {
		if constexpr (std::is_reference_v<Callable> || !std::is_convertible_v<Callable, bool>) {
			if (!m_canceled)
				detail::apply(m_callable, m_arguments);
		} else {
			if (!m_canceled && m_callable)
				detail::apply(m_callable, m_arguments);
		}
	}
	void cancel() {
		m_canceled = true;
	}
private:
	Callable m_callable;
	std::tuple<detail::UnwrapAndDecay<Args>...> m_arguments;
	bool m_canceled;
};
template<typename Callable, typename... Args> Scoped(Callable, Args &&...) -> Scoped<Callable, Args...>;
}
#endif /* GPICK_COMMON_SCOPED_H_ */
