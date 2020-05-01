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

#include <boost/test/unit_test.hpp>
#include "common/Span.h"
#include <vector>
using namespace common;
BOOST_AUTO_TEST_SUITE(span);
BOOST_AUTO_TEST_CASE(iteration) {
	size_t j = 0;
	std::string result;
	for (auto i: Span<const char>("test", 4)) {
		j++;
		result += i;
	}
	BOOST_CHECK_EQUAL(j, 4);
	BOOST_CHECK_EQUAL(result, "test");
}
BOOST_AUTO_TEST_CASE(constIteration) {
	size_t j = 0;
	std::string result;
	auto span = Span<const char>("test", 4);
	const auto &values = span;
	for (auto i: values) {
		j++;
		result += i;
	}
	BOOST_CHECK_EQUAL(j, 4);
	BOOST_CHECK_EQUAL(result, "test");
}
BOOST_AUTO_TEST_CASE(size) {
	BOOST_CHECK_EQUAL(Span<const char>("value", 5).size(), 5);
}
BOOST_AUTO_TEST_CASE(vectorInitialization) {
	auto span = Span<const char>("value", 5);
	std::vector<char> value(span.begin(), span.end());
	BOOST_CHECK_EQUAL(value.size(), 5);
}
BOOST_AUTO_TEST_CASE(startEndInitialization) {
	bool bools[] = { false, true, false, true, false };
	auto span = Span<bool>(bools, bools + 5);
	BOOST_CHECK_EQUAL(span.size(), 5);
}
BOOST_AUTO_TEST_SUITE_END()
