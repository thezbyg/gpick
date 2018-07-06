#include <boost/test/unit_test.hpp>
#include "lua/Script.h"
extern "C"{
#include <lualib.h>
#include <lauxlib.h>
}
using namespace std;
using namespace lua;

static int test(lua_State *L)
{
	lua_pushstring(L, "ok");
	return 1;
}
BOOST_AUTO_TEST_CASE(register_extension)
{
	Script script;
	bool status = script.registerExtension("test", [](Script &script){
		static const struct luaL_Reg functions[] = {
			{"test", test},
			{nullptr, nullptr}
		};
		luaL_newlib(script, functions);
		return 1;
	});
	BOOST_CHECK(status == true);
	status = script.loadCode("test = require(\"gpick/test\")\nreturn test.test()");
	BOOST_CHECK(status == true);
	status = script.run(0, 1);
	BOOST_CHECK(status == true);
	string return_value = script.getString(-1);
	BOOST_CHECK(return_value == "ok");
}
BOOST_AUTO_TEST_CASE(register_nullptr_extension)
{
	Script script;
	bool status = script.registerExtension(nullptr, [](Script &script){
		lua_State *L = script;
		lua_newtable(L);
		lua_pushstring(L, "ok");
		lua_setfield(L, -2, "test");
		return 1;
	});
	BOOST_CHECK(status == true);
	status = script.loadCode("test = require(\"gpick\")\nreturn test.test");
	BOOST_CHECK(status == true);
	status = script.run(0, 1);
	BOOST_CHECK(status == true);
	string return_value = script.getString(-1);
	BOOST_CHECK(return_value == "ok");
}
