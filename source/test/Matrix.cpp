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
#include "math/Matrix.h"
#include <iostream>
namespace math {
static std::ostream &operator<<(std::ostream &stream, const math::Matrix3d &matrix) {
	for (size_t i = 0; i < math::Matrix3d::Size * math::Matrix3d::Size; i++) {
		if (i != 0)
			stream << ", ";
		stream << matrix.flatData[i];
	}
	return stream;
}
}
BOOST_AUTO_TEST_SUITE(matrix)
const static math::Matrix3d zero = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
const static math::Matrix3d identity = { 1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0 };
BOOST_AUTO_TEST_CASE(determinant) {
	BOOST_CHECK_EQUAL(zero.determinant(), 0.0);
	BOOST_CHECK_EQUAL(identity.determinant(), 1.0);
	BOOST_CHECK_EQUAL(math::Matrix3d(1.0, 2.0, 3.0, 0.0, 1.0, 4.0, 5.0, 6.0, 0.0).determinant(), 1.0);
}
BOOST_AUTO_TEST_CASE(multiplication) {
	BOOST_CHECK_EQUAL(identity * zero, zero);
	BOOST_CHECK_EQUAL(zero * identity, zero);
	BOOST_CHECK_EQUAL(identity * identity, identity);
}
BOOST_AUTO_TEST_CASE(inverse) {
	BOOST_CHECK_EQUAL(*identity.inverse(), identity);
	BOOST_CHECK_EQUAL(*math::Matrix3d(1.0, 2.0, 3.0, 0.0, 1.0, 4.0, 5.0, 6.0, 0.0).inverse(), math::Matrix3d(-24.0, 18.0, 5.0, 20.0, -15.0, -4.0, -5.0, 4.0, 1.0));
}
BOOST_AUTO_TEST_SUITE_END()
