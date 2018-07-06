#include <boost/test/unit_test.hpp>
#include "Format.h"
BOOST_AUTO_TEST_CASE(format_empty)
{
	BOOST_CHECK_EQUAL(format(""), "");
}
BOOST_AUTO_TEST_CASE(format_missing_value)
{
	BOOST_CHECK_EQUAL(format("{}"), "");
}
BOOST_AUTO_TEST_CASE(format_useless_value)
{
	BOOST_CHECK_EQUAL(format("", 123), "");
}
BOOST_AUTO_TEST_CASE(format_int)
{
	BOOST_CHECK_EQUAL(format("Hello world {}", 123), "Hello world 123");
}
BOOST_AUTO_TEST_CASE(format_string)
{
	BOOST_CHECK_EQUAL(format("Hello {}", "world"), "Hello world");
}
BOOST_AUTO_TEST_CASE(format_mixed)
{
	BOOST_CHECK_EQUAL(format("Hello {} {}", "world", 123), "Hello world 123");
}
BOOST_AUTO_TEST_CASE(format_unterminated)
{
	BOOST_CHECK_EQUAL(format("Hello {", 123), "Hello {");
}
