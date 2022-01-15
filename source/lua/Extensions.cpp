/*
 * Copyright (c) 2009-2017, Albertas Vyšniauskas
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

#include "Extensions.h"
#include "Script.h"
#include "Color.h"
#include "ColorObject.h"
#include "DynvSystem.h"
#include "Layout.h"
#include "I18N.h"
#include "GlobalState.h"
#include "Callbacks.h"
#include "../GlobalState.h"
#include "../layout/Layouts.h"
#include "../layout/Layout.h"
#include "../Converters.h"
#include "../Converter.h"
#include "version/Version.h"
#include <lualib.h>
#include <lauxlib.h>
namespace lua
{
static void checkArgumentIsFunctionOrNil(lua_State *L, int index)
{
	auto type = lua_type(L, index);
	bool type_matches = type == LUA_TFUNCTION || type == LUA_TNIL;
	luaL_argcheck(L, type_matches, index, "function or nil expected");
}
static int addLayout(lua_State *L)
{
	const char *name = luaL_checkstring(L, 2);
	const char *label = luaL_checkstring(L, 3);
	checkArgumentIsFunctionOrNil(L, 4);
	int mask = luaL_optinteger(L, 5, 0);
	getGlobalState(L).layouts().add(new layout::Layout(name, label, mask, Ref(L, 4)));
	return 0;
}
static int addConverter(lua_State *L)
{
	const char *name = luaL_checkstring(L, 2);
	const char *label = luaL_checkstring(L, 3);
	checkArgumentIsFunctionOrNil(L, 4);
	if (lua_gettop(L) >= 5) checkArgumentIsFunctionOrNil(L, 5);
	if (lua_gettop(L) == 4)
		getGlobalState(L).converters().add(new Converter(name, label, Ref(L, 4), Ref()));
	else if (lua_gettop(L) >= 5)
		getGlobalState(L).converters().add(new Converter(name, label, Ref(L, 4), Ref(L, 5)));
	return 0;
}
static int setOptionChangeCallback(lua_State *L)
{
	getGlobalState(L).callbacks().optionChange(Ref(L, 2));
	return 0;
}
static int setComponentToTextCallback(lua_State *L)
{
	getGlobalState(L).callbacks().componentToText(Ref(L, 2));
	return 0;
}
static const struct luaL_Reg functions[] =
{
	{"addLayout", addLayout},
	{"addConverter", addConverter},
	{"setComponentToTextCallback", setComponentToTextCallback},
	{"setOptionChangeCallback", setOptionChangeCallback},
	{nullptr, nullptr}
};
void registerAll(lua_State *L, GlobalState &global_state)
{
	Script script(L);
	script.registerExtension("color", registerColor);
	script.registerExtension("colorObject", registerColorObject);
	script.registerExtension("dynvSystem", registerDynvSystem);
	script.registerExtension("layout", registerLayout);
	script.registerExtension(nullptr, [](lua_State *L){
		luaL_newlib(L, functions);
		lua_pushstring(L, version::version);
		lua_setfield(L, -2, "version");
		lua_pushinteger(L, version::revision);
		lua_setfield(L, -2, "revision");
		lua_pushstring(L, version::hash);
		lua_setfield(L, -2, "hash");
		lua_pushstring(L, version::date);
		lua_setfield(L, -2, "date");
		lua_pushcclosure(L, getText, 0);
		lua_setfield(L, -2, "_");
		return 1;
	});
	setGlobalState(L, global_state);
}
}
