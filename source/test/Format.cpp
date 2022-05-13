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
#include "common/Format.h"
using namespace common;
namespace {
struct CustomType {
};
static std::string to_string(const CustomType &) {
	return "Hello world! Hello world! Hello world!";
}
}
BOOST_AUTO_TEST_SUITE(commonFormat)
BOOST_AUTO_TEST_CASE(empty) {
	BOOST_CHECK_EQUAL(format(""), "");
}
BOOST_AUTO_TEST_CASE(missingValue) {
	BOOST_CHECK_EQUAL(format("{}"), "");
}
BOOST_AUTO_TEST_CASE(uselessValue) {
	BOOST_CHECK_EQUAL(format("", 123), "");
}
BOOST_AUTO_TEST_CASE(valueInt) {
	BOOST_CHECK_EQUAL(format("Hello world {}", 123), "Hello world 123");
}
BOOST_AUTO_TEST_CASE(valueString) {
	BOOST_CHECK_EQUAL(format("Hello {}", "world"), "Hello world");
}
BOOST_AUTO_TEST_CASE(valuesMixed) {
	BOOST_CHECK_EQUAL(format("Hello {} {}", "world", 123), "Hello world 123");
}
BOOST_AUTO_TEST_CASE(unterminated) {
	BOOST_CHECK_EQUAL(format("Hello {", 123), "Hello {");
}
BOOST_AUTO_TEST_CASE(valueLongString) {
	const std::string value = "Hello world! Hello world! Hello world!";
	BOOST_CHECK_EQUAL(format("{}", value), value);
}
BOOST_AUTO_TEST_CASE(valueCustomType) {
	BOOST_CHECK_EQUAL(format("{}", CustomType()), "Hello world! Hello world! Hello world!");
}
BOOST_AUTO_TEST_SUITE_END()
