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

#ifndef GPICK_COMMON_GUARD_H_
#define GPICK_COMMON_GUARD_H_
#include "Scoped.h"
namespace common {
template<typename Callable, typename... Args>
struct Guard;
template<typename Callable, typename... Args>
struct Guard {
	Guard(bool guarded, Callable callable, Args... params):
		m_guarded(guarded),
		m_canceled(false),
		m_callable(callable),
		m_arguments(std::forward_as_tuple(params...)) {
	}
	Guard(Guard &&guard):
		m_guarded(guard.m_guarded),
		m_canceled(guard.m_canceled),
		m_callable(std::move(guard.m_callable)),
		m_arguments(std::move(guard.m_arguments)) {
		guard.m_canceled = true;
	}
	Guard(const Guard &) = delete;
	Guard &operator=(const Guard &) = delete;
	~Guard() {
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
	bool m_guarded, m_canceled;
	Callable m_callable;
	std::tuple<detail::UnwrapAndDecay<Args>...> m_arguments;
};
template<typename Callable, typename... Args> Guard(bool, Callable, Args &&...) -> Guard<Callable, Args...>;
}
#endif /* GPICK_COMMON_GUARD_H_ */
