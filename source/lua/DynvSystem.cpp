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

#include "DynvSystem.h"
#include <lualib.h>
#include <lauxlib.h>
namespace lua {
dynv::Ref checkDynvSystem(lua_State *L, int index) {
	void *ud = luaL_checkudata(L, index, "dynvSystem");
	luaL_argcheck(L, ud != nullptr, index, "`dynvSystem' expected");
	return *reinterpret_cast<dynv::Ref*>(ud);
}
int pushDynvSystem(lua_State *L, const dynv::Ref &system) {
	new (lua_newuserdata(L, sizeof(dynv::Ref))) dynv::Ref(system);
	luaL_getmetatable(L, "dynvSystem");
	lua_setmetatable(L, -2);
	return 1;
}
int getString(lua_State *L) {
	auto system = checkDynvSystem(L, 1);
	if (!system) {
		lua_pushnil(L);
		return 1;
	}
	const char *name = luaL_checkstring(L, 2);
	const char *defaultValue = luaL_checkstring(L, 3);
	lua_pushstring(L, system->getString(name, defaultValue).c_str());
	return 1;
}
static const struct luaL_Reg dynvSystemMembers[] = {
	{"getString", getString},
	{nullptr, nullptr}
};
int registerDynvSystem(lua_State *L) {
	luaL_newmetatable(L, "dynvSystem");
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	luaL_setfuncs(L, dynvSystemMembers, 0);
	lua_pop(L, 1);
	return 0;
}
}
