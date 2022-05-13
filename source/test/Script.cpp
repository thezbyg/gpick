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
#include "lua/Script.h"
#include "common/Scoped.h"
#include <lualib.h>
#include <lauxlib.h>
using namespace lua;
static int test(lua_State *L) {
	lua_pushstring(L, "ok");
	return 1;
}
static int error(lua_State *L) {
	lua_getglobal(L, "cleanupOnError");
	auto &cleanupOnError = *reinterpret_cast<bool *>(lua_touserdata(L, -1));
	lua_pop(L, 1);
	common::Scoped<std::function<void()>> callOnScopeEnd([&cleanupOnError]() {
		cleanupOnError = true;
	});
	luaL_error(L, "error");
	return 0;
}
BOOST_AUTO_TEST_SUITE(script)
BOOST_AUTO_TEST_CASE(registerExtension) {
	Script script;
	bool status = script.registerExtension("test", [](Script &script) {
		static const struct luaL_Reg functions[] = {
			{ "test", test },
			{ nullptr, nullptr }
		};
		luaL_newlib(script, functions);
		return 1;
	});
	BOOST_CHECK(status == true);
	status = script.loadCode("test = require(\"gpick/test\")\nreturn test.test()");
	BOOST_CHECK(status == true);
	status = script.run(0, 1);
	BOOST_CHECK(status == true);
	std::string returnValue = script.getString(-1);
	BOOST_CHECK(returnValue == "ok");
}
BOOST_AUTO_TEST_CASE(registerNullptrExtension) {
	Script script;
	bool status = script.registerExtension(nullptr, [](Script &script) {
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
	std::string returnValue = script.getString(-1);
	BOOST_CHECK(returnValue == "ok");
}
BOOST_AUTO_TEST_CASE(errorHandling) {
	Script script;
	lua_State *L = script;
	bool cleanupOnError = false;
	lua_pushlightuserdata(L, &cleanupOnError);
	lua_setglobal(L, "cleanupOnError");
	bool status = script.registerExtension("test", [](Script &script) {
		static const struct luaL_Reg functions[] = {
			{ "error", error },
			{ nullptr, nullptr }
		};
		luaL_newlib(script, functions);
		return 1;
	});
	BOOST_CHECK(status == true);
	status = script.loadCode("test = require(\"gpick/test\")\nreturn test.error()");
	BOOST_CHECK(status == true);
	status = script.run(0, 0);
	BOOST_CHECK(status == false);
	BOOST_CHECK(cleanupOnError == true);
}
BOOST_AUTO_TEST_SUITE_END()
