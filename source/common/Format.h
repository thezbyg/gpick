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

#ifndef GPICK_COMMON_FORMAT_H_
#define GPICK_COMMON_FORMAT_H_
#include "Span.h"
#include "StringOrView.h"
#include <string>
#include <string_view>
#include <array>
#include <type_traits>
#include <utility>
namespace common {
namespace detail {
std::string format(const char *format, const Span<StringOrView> values);
template<typename T>
StringOrView asStringOrView(T &&value) {
	if constexpr (std::is_same_v<std::decay_t<T>, std::string> || std::is_same_v<std::decay_t<T>, std::string_view> || std::is_same_v<std::remove_cv_t<std::remove_pointer_t<std::decay_t<std::remove_reference_t<T>>>>, char>) {
		return StringOrView(std::string_view(value));
	} else {
		using std::to_string;
		return StringOrView(to_string(value));
	}
}
}
template<typename... Args>
std::string format(const char *format, Args &&... args) {
	std::array<StringOrView, sizeof...(Args)> values = { detail::asStringOrView(std::forward<Args>(args))... };
	return detail::format(format, Span<StringOrView>(values.data(), values.size()));
}
}
#endif /* GPICK_COMMON_FORMAT_H_ */
