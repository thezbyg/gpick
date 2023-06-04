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

#include "Format.h"
#include <cstddef>
namespace common {
namespace detail {
std::string format(const char *format, const Span<StringOrView> values) {
	size_t maxLength = 0;
	for (auto &v: values) {
		maxLength += v.length();
	}
	char previousChar = 0;
	size_t i;
	for (i = 0; format[i]; ++i) {
		if (format[i] == '}' && previousChar == '{') {
			maxLength -= 2;
		}
		previousChar = format[i];
	}
	maxLength += i;
	std::string result;
	result.reserve(maxLength);
	previousChar = 0;
	size_t argumentIndex = 0;
	for (i = 0; format[i]; ++i) {
		if (format[i] == '}' && previousChar == '{') {
			result.pop_back();
			if (argumentIndex < values.size()) {
				auto value = values[argumentIndex].view();
				for (size_t j = 0; j < value.length(); ++j) {
					result.push_back(value[j]);
				}
			}
			argumentIndex++;
		} else {
			result.push_back(format[i]);
		}
		previousChar = format[i];
	}
	return result;
}
}
}
