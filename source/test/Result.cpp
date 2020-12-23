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
#include "common/Result.h"
using namespace common;
BOOST_AUTO_TEST_SUITE(result);
BOOST_AUTO_TEST_CASE(booleanValue) {
	Result<bool, void> resultEmpty;
	Result<bool, void> resultTrue(true);
	Result<bool, void> resultFalse(false);
	BOOST_CHECK(!resultEmpty);
	BOOST_CHECK(resultTrue);
	BOOST_CHECK(resultFalse);
	BOOST_CHECK(resultTrue.value());
	BOOST_CHECK(!resultFalse.value());
}
BOOST_AUTO_TEST_CASE(booleanError) {
	Result<void, bool> resultEmpty;
	Result<void, bool> errorTrue(true);
	Result<void, bool> errorFalse(false);
	BOOST_CHECK(resultEmpty);
	BOOST_CHECK(!errorTrue);
	BOOST_CHECK(!errorFalse);
	BOOST_CHECK(errorTrue.error());
	BOOST_CHECK(!errorFalse.error());
}
BOOST_AUTO_TEST_CASE(booleanValueAndError) {
	Result<bool, bool> resultEmpty;
	Result<bool, bool> resultTrue(true, true);
	Result<bool, bool> resultFalse(true, false);
	Result<bool, bool> errorTrue(false, true);
	Result<bool, bool> errorFalse(false, false);
	BOOST_CHECK(!resultEmpty);
	BOOST_CHECK(resultTrue);
	BOOST_CHECK(resultFalse);
	BOOST_CHECK(resultTrue.value());
	BOOST_CHECK(!resultFalse.value());
	BOOST_CHECK(!errorTrue);
	BOOST_CHECK(!errorFalse);
	BOOST_CHECK(errorTrue.error());
	BOOST_CHECK(!errorFalse.error());
}
struct Movable {
	Movable() {
	}
	Movable(const Movable &) = delete;
	Movable(Movable &&movable) {
	}
	Movable &operator=(const Movable &) = delete;
	Movable &operator=(Movable &&movable) {
		return *this;
	}
};
static Result<Movable, void> returnMovableValue() {
	return Result<Movable, void>(Movable());
}
static Result<void, Movable> returnMovableError() {
	return Result<void, Movable>(Movable());
}
static Result<Movable, Movable> returnMovableValueAndError(bool status) {
	return Result<Movable, Movable>(status, Movable());
}
static Result<bool, Movable> returnMovableErrorWithValue() {
	return Result<bool, Movable>(Movable());
}
static Result<Movable, bool> returnMovableValueWithError() {
	return Result<Movable, bool>(Movable());
}
BOOST_AUTO_TEST_CASE(movable) {
	BOOST_CHECK(returnMovableValue());
	BOOST_CHECK(!returnMovableError());
	BOOST_CHECK(returnMovableValueAndError(true));
	BOOST_CHECK(!returnMovableValueAndError(false));
	BOOST_CHECK(returnMovableValueWithError());
	BOOST_CHECK(!returnMovableErrorWithValue());
}
BOOST_AUTO_TEST_SUITE_END()
