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

#ifndef GPICK_DYNV_TYPES_H_
#define GPICK_DYNV_TYPES_H_
#include "common/Ref.h"
#include <iosfwd>
#include <type_traits>
namespace dynv {
struct Map;
namespace types {
enum class ValueType : uint8_t {
	unknown,
	map,
	basicBool,
	basicFloat,
	basicInt32,
	color,
	string,
};
struct KnownHandler {
	std::string name;
	ValueType type;
};
template<typename T> const KnownHandler &typeHandler();
namespace xml {
template<typename T, typename std::enable_if_t<std::is_arithmetic<T>::value, int> = 0> bool write(std::ostream &stream, T value);
template<typename T, typename std::enable_if_t<!std::is_arithmetic<T>::value, int> = 0> bool write(std::ostream &stream, const T &value);
}
namespace binary {
template<typename T, typename std::enable_if_t<std::is_arithmetic<T>::value, int> = 0> bool write(std::ostream &stream, T value);
template<typename T, typename std::enable_if_t<!std::is_arithmetic<T>::value, int> = 0> bool write(std::ostream &stream, const T &value);
template<typename T> T read(std::istream &stream);
}
ValueType stringToType(const char *value);
ValueType stringToType(const std::string &value);
}
}
#endif /* GPICK_DYNV_TYPES_H_ */
