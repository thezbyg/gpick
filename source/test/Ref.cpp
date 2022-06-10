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

#include <boost/test/unit_test.hpp>
#include "common/Ref.h"
#include <ostream>
namespace common {
template<typename T>
std::ostream &operator<<(std::ostream &stream, const common::Ref<T> &reference) {
	stream << typeid(T).name() << " [" << reference.references() << ']';
	return stream;
}
}
BOOST_AUTO_TEST_SUITE(commonRef);
struct Base: public common::Ref<Base>::Counter {
};
struct Derived: public Base {
};
BOOST_AUTO_TEST_CASE(basic) {
	common::Ref<Base> base(new Base());
	BOOST_CHECK_EQUAL(base.references(), 1);
	{
		common::Ref<Base> baseRef = base;
		BOOST_CHECK_EQUAL(base.references(), 2);
		BOOST_CHECK_EQUAL(baseRef.references(), 2);
		BOOST_CHECK_EQUAL(base, baseRef);
	}
	BOOST_CHECK_EQUAL(base.references(), 1);
}
BOOST_AUTO_TEST_CASE(conversionToBase) {
	common::Ref<Derived> derived(new Derived());
	BOOST_CHECK_EQUAL(derived.references(), 1);
	common::Ref<Base> base(derived);
	BOOST_CHECK_EQUAL(derived.references(), 2);
	BOOST_CHECK_EQUAL(base.references(), 2);
	BOOST_CHECK_EQUAL(base, derived);
}
BOOST_AUTO_TEST_CASE(nullReference) {
	common::Ref<Base> base(new Base());
	base = common::nullRef;
	BOOST_CHECK(!base);
	BOOST_CHECK_EQUAL(base.references(), 0);
}
BOOST_AUTO_TEST_CASE(wrapUnwrap) {
	Base *base = new Base();
	{
		common::Ref<Base> ref(base);
		BOOST_CHECK_EQUAL(base->references(), 1);
		base->reference();
		BOOST_CHECK_EQUAL(base->references(), 2);
	}
	{
		auto ref = common::wrapInRef(base);
		BOOST_CHECK_EQUAL(base->references(), 2);
	}
	{
		common::Ref<Base> ref(base);
		Base *unwrapped = ref.unwrap();
		BOOST_CHECK_EQUAL(ref.references(), 0);
		BOOST_CHECK_EQUAL(unwrapped->references(), 1);
	}
	delete base;
}
BOOST_AUTO_TEST_SUITE_END()
