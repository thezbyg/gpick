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

#include "Convert.h"
#include <type_traits>
#include <boost/spirit/include/qi.hpp>
namespace qi = boost::spirit::qi;
namespace common {
template<typename T>
std::optional<T> convertOptional(std::string_view value) {
	T result = 0;
	if constexpr (std::is_same_v<T, float>) {
		if (!qi::parse(value.begin(), value.end(), qi::float_ >> qi::eoi, result))
			return std::nullopt;
	}else if constexpr (std::is_same_v<T, double>) {
		if (!qi::parse(value.begin(), value.end(), qi::double_ >> qi::eoi, result))
			return std::nullopt;
	} else if constexpr (std::is_unsigned_v<T>) {
		if (!qi::parse(value.begin(), value.end(), qi::ulong_ >> qi::eoi, result))
			return std::nullopt;
	} else {
		if (!qi::parse(value.begin(), value.end(), qi::long_ >> qi::eoi, result))
			return std::nullopt;
	}
	return result;
}
template<typename T>
T convert(std::string_view value, T defaultValue) {
	auto result = convertOptional<T>(value);
	if (!result)
		return defaultValue;
	return *result;
}
template uint16_t convert<uint16_t>(std::string_view value, uint16_t defaultValue);
template uint32_t convert<uint32_t>(std::string_view value, uint32_t defaultValue);
template uint64_t convert<uint64_t>(std::string_view value, uint64_t defaultValue);
template int16_t convert<int16_t>(std::string_view value, int16_t defaultValue);
template int32_t convert<int32_t>(std::string_view value, int32_t defaultValue);
template int64_t convert<int64_t>(std::string_view value, int64_t defaultValue);
template float convert<float>(std::string_view value, float defaultValue);
template double convert<double>(std::string_view value, double defaultValue);
template std::optional<uint16_t> convertOptional<uint16_t>(std::string_view value);
template std::optional<uint32_t> convertOptional<uint32_t>(std::string_view value);
template std::optional<uint64_t> convertOptional<uint64_t>(std::string_view value);
template std::optional<int16_t> convertOptional<int16_t>(std::string_view value);
template std::optional<int32_t> convertOptional<int32_t>(std::string_view value);
template std::optional<int64_t> convertOptional<int64_t>(std::string_view value);
template std::optional<float> convertOptional<float>(std::string_view value);
template std::optional<double> convertOptional<double>(std::string_view value);
}
