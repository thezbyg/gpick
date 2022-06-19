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

#ifndef GPICK_COMMON_MATCH_H_
#define GPICK_COMMON_MATCH_H_
#include <string_view>
namespace common {
template<typename Type, std::size_t TypeCount>
Type &matchById(Type (&types)[TypeCount], std::string_view id) {
	static_assert(TypeCount > 0, "At least one type is required");
	for (std::size_t index = 0; index < TypeCount; ++index) {
		if (types[index].id == id) {
			return types[index];
		}
	}
	return types[0];
}
template<typename Type, std::size_t TypeCount>
Type &matchById(Type (&types)[TypeCount], std::string_view id, Type &defaultValue) {
	for (std::size_t index = 0; index < TypeCount; ++index) {
		if (types[index].id == id) {
			return types[index];
		}
	}
	return defaultValue;
}
template<typename Type, std::size_t TypeCount, typename Callback>
Type &matchById(Type (&types)[TypeCount], std::string_view id, Callback &&callback) {
	for (std::size_t index = 0; index < TypeCount; ++index) {
		if (types[index].id == id) {
			return types[index];
		}
	}
	return callback(id);
}
template<typename Container>
typename Container::value_type &matchById(Container &&types, std::string_view id) {
	for (std::size_t index = 0, end = types.size(); index < end; ++index) {
		if (types[index].id == id) {
			return types[index];
		}
	}
	return types[0];
}
template<typename Container>
typename Container::value_type &matchById(Container &&types, std::string_view id, typename Container::value_type &defaultValue) {
	for (std::size_t index = 0, end = types.size(); index < end; ++index) {
		if (types[index].id == id) {
			return types[index];
		}
	}
	return defaultValue;
}
template<typename Container, typename Callback>
typename Container::value_type &matchById(Container &&types, std::string_view id, Callback &&callback) {
	for (std::size_t index = 0, end = types.size(); index < end; ++index) {
		if (types[index].id == id) {
			return types[index];
		}
	}
	return callback(id);
}
}
#endif /* GPICK_COMMON_MATCH_H_ */
