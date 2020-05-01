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
#include <string>
#include <vector>
namespace common {
template<typename T>
std::string as_string(T value) {
	return std::to_string(value);
}
template<> std::string as_string<const std::string &>(const std::string &value);
template<> std::string as_string<const char *>(const char *value);
template<> std::string as_string<int>(int value);
template<typename... Args>
std::string format(const char *format, const Args &... args) {
	std::vector<std::string> values = { as_string(args)... };
	size_t max_length = 0;
	for (auto &v: values) {
		max_length += v.length();
	}
	char previous_char = 0;
	size_t i;
	for (i = 0; format[i]; ++i) {
		if (format[i] == '}' && previous_char == '{') {
			max_length -= 2;
		}
		previous_char = format[i];
	}
	max_length += i;
	std::string result;
	result.reserve(max_length);
	previous_char = 0;
	size_t argument_index = 0;
	for (i = 0; format[i]; ++i) {
		if (format[i] == '}' && previous_char == '{') {
			result.pop_back();
			if (argument_index < values.size()) {
				auto &value = values[argument_index];
				for (size_t j = 0; j < value.length(); ++j) {
					result.push_back(value[j]);
				}
			}
			argument_index++;
		} else {
			result.push_back(format[i]);
		}
		previous_char = format[i];
	}
	return result;
}
}
#endif /* GPICK_COMMON_FORMAT_H_ */
