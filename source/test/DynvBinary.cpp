/*
 * Copyright (c) 2009-2021, Albertas Vyšniauskas
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
#include <iostream>
#include <sstream>
#include "dynv/Types.h"
#include "Color.h"
static std::ostream &operator<<(std::ostream &stream, const Color &color) {
	stream << color[0] << "," << color[1] << "," << color[2] << "," << color[3];
	return stream;
}
BOOST_AUTO_TEST_SUITE(dynvBinary);
BOOST_AUTO_TEST_CASE(serialize) {
	using namespace dynv::types::binary;
	std::stringstream output(std::ios::out | std::ios::binary);
	BOOST_CHECK(write<bool>(output, true));
	BOOST_CHECK(write<uint8_t>(output, 1u));
	BOOST_CHECK(write<int32_t>(output, 1));
	BOOST_CHECK(write<uint32_t>(output, 1u));
	BOOST_CHECK(write<float>(output, 1.0f));
	BOOST_CHECK(write<Color>(output, Color(0.1f, 0.2f, 0.3f, 0.4f)));
	BOOST_CHECK(write<std::string>(output, ""));
	BOOST_CHECK(write<std::string>(output, "Test"));
	auto length = output.str().length();
	BOOST_CHECK_EQUAL(length, 46);
}
BOOST_AUTO_TEST_CASE(deserialize) {
	using namespace dynv::types::binary;
	std::stringstream output(std::ios::out | std::ios::in | std::ios::binary);
	BOOST_CHECK(write<bool>(output, true));
	BOOST_CHECK(write<uint8_t>(output, 1u));
	BOOST_CHECK(write<int32_t>(output, 1));
	BOOST_CHECK(write<uint32_t>(output, 1u));
	BOOST_CHECK(write<float>(output, 1.0f));
	BOOST_CHECK(write<Color>(output, Color(0.1f, 0.2f, 0.3f, 0.4f)));
	BOOST_CHECK(write<std::string>(output, ""));
	BOOST_CHECK(write<std::string>(output, "Test"));
	auto length = output.str().length();
	BOOST_CHECK_EQUAL(length, 46);
	BOOST_CHECK_EQUAL(read<bool>(output), true);
	BOOST_CHECK_EQUAL(read<uint8_t>(output), 1u);
	BOOST_CHECK_EQUAL(read<int32_t>(output), 1);
	BOOST_CHECK_EQUAL(read<uint32_t>(output), 1u);
	BOOST_CHECK_EQUAL(read<float>(output), 1.0f);
	BOOST_CHECK_EQUAL(read<Color>(output), Color(0.1f, 0.2f, 0.3f, 0.4f));
	BOOST_CHECK_EQUAL(read<std::string>(output), "");
	BOOST_CHECK_EQUAL(read<std::string>(output), "Test");
}
BOOST_AUTO_TEST_SUITE_END()
