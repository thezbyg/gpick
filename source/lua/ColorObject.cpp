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

#include "ColorObject.h"
#include "Color.h"
#include "Script.h"
#include "../ColorObject.h"
#include <lualib.h>
#include <lauxlib.h>
namespace lua
{
static int newColorObject(lua_State *L)
{
	ColorObject** c = reinterpret_cast<ColorObject**>(lua_newuserdata(L, sizeof(ColorObject*)));
	luaL_getmetatable(L, "colorObject");
	lua_setmetatable(L, -2);
	*c = nullptr;
	return 1;
}
ColorObject* checkColorObject(lua_State *L, int index)
{
	void *ud = luaL_checkudata(L, index, "colorObject");
	luaL_argcheck(L, ud != nullptr, index, "`colorObject' expected");
	return *reinterpret_cast<ColorObject**>(ud);
}
int pushColorObject(lua_State *L, ColorObject* color_object)
{
	ColorObject** c = reinterpret_cast<ColorObject**>(lua_newuserdata(L, sizeof(ColorObject*)));
	luaL_getmetatable(L, "colorObject");
	lua_setmetatable(L, -2);
	*c = color_object;
	return 1;
}
int getColor(lua_State *L)
{
	ColorObject *color_object = checkColorObject(L, 1);
	const Color &tmp = color_object->getColor();
	pushColor(L, tmp);
	return 1;
}
int setColor(lua_State *L)
{
	ColorObject *color_object = checkColorObject(L, 1);
	const Color &color = checkColor(L, 2);
	color_object->setColor(color);
	return 0;
}
int getName(lua_State *L)
{
	ColorObject *color_object = checkColorObject(L, 1);
	lua_pushstring(L, color_object->getName().c_str());
	return 1;
}
static const struct luaL_Reg color_object_functions[] =
{
	{"new", newColorObject},
	{nullptr, nullptr}
};
static const struct luaL_Reg color_object_members[] = {
	{"getColor", getColor},
	{"setColor", setColor},
	{"getName", getName},
	{nullptr, nullptr}
};
int registerColorObject(lua_State *L)
{
	luaL_newmetatable(L, "colorObject");
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	luaL_setfuncs(L, color_object_members, 0);
	lua_pop(L, 1);
	luaL_newlib(L, color_object_functions);
	return 1;
}
}
