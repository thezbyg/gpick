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

#include "Layout.h"
#include "Script.h"
#include "Color.h"
#include "GlobalState.h"
#include "../layout/Box.h"
#include "../layout/System.h"
#include "../layout/Layout.h"
extern "C"{
#include <lualib.h>
#include <lauxlib.h>
}
#include <typeinfo>
#include <iostream>
using namespace std;
using namespace layout;
namespace lua
{
static int newLayoutStyle(lua_State *L)
{
	const char *name = luaL_checkstring(L, 2);
	Color &color = checkColor(L, 3);
	double font_size = luaL_optnumber(L, 4, 1.0);
	Style **c = static_cast<Style**>(lua_newuserdata(L, sizeof(Style*)));
	luaL_getmetatable(L, "layoutStyle");
	lua_setmetatable(L, -2);
	*c = new Style(name, &color, static_cast<float>(font_size));
	return 1;
}
Style *checkLayoutStyle(lua_State *L, int index)
{
	Style **c = static_cast<Style**>(luaL_checkudata(L, index, "layoutStyle"));
	luaL_argcheck(L, c != nullptr, index, "`layoutStyle' expected");
	return *c;
}
int pushLayoutStyle(lua_State *L, Style *style)
{
	Style **c = static_cast<Style**>(lua_newuserdata(L, sizeof(Style*)));
	luaL_getmetatable(L, "layoutStyle");
	lua_setmetatable(L, -2);
	*c = style;
	return 1;
}
int styleGc(lua_State *L)
{
	Style *style = checkLayoutStyle(L, 1);
	Style::unref(style);
	return 0;
}
int styleLabel(lua_State *L)
{
	Style *style = checkLayoutStyle(L, 1);
	if (lua_type(L, 2) == LUA_TSTRING){
		const char *name = luaL_checkstring(L, 2);
		style->label = name;
		return 0;
	}else{
		lua_pushstring(L, style->label.c_str());
		return 1;
	}
}
static int newLayoutBox(lua_State *L)
{
	const char *name = luaL_checkstring(L, 2);
	double x = luaL_checknumber(L, 3);
	double y = luaL_checknumber(L, 4);
	double w = luaL_checknumber(L, 5);
	double h = luaL_checknumber(L, 6);
	Box **c = static_cast<Box**>(lua_newuserdata(L, sizeof(Box*)));
	luaL_getmetatable(L, "layout");
	lua_setmetatable(L, -2);
	*c = new Box(name, static_cast<float>(x), static_cast<float>(y), static_cast<float>(w), static_cast<float>(h));
	return 1;
}
static int newLayoutFill(lua_State *L)
{
	const char *name = luaL_checkstring(L, 2);
	double x = luaL_checknumber(L, 3);
	double y = luaL_checknumber(L, 4);
	double w = luaL_checknumber(L, 5);
	double h = luaL_checknumber(L, 6);
	Style *style = checkLayoutStyle(L, 7);
	Box **c = static_cast<Box**>(lua_newuserdata(L, sizeof(Box*)));
	luaL_getmetatable(L, "layout");
	lua_setmetatable(L, -2);
	Fill *e = new Fill(name, static_cast<float>(x), static_cast<float>(y), static_cast<float>(w), static_cast<float>(h));
	e->setStyle(style);
	*c = e;
	return 1;
}
static int newLayoutText(lua_State *L)
{
	const char *name = luaL_checkstring(L, 2);
	double x = luaL_checknumber(L, 3);
	double y = luaL_checknumber(L, 4);
	double w = luaL_checknumber(L, 5);
	double h = luaL_checknumber(L, 6);
	Style *style = nullptr;
	if (lua_type(L, 7) != LUA_TNIL){
		style = checkLayoutStyle(L, 7);
	}
	const char *text = luaL_checkstring(L, 8);
	Box **c = static_cast<Box**>(lua_newuserdata(L, sizeof(Box*)));
	luaL_getmetatable(L, "layout");
	lua_setmetatable(L, -2);
	Text *e = new Text(name, static_cast<float>(x), static_cast<float>(y), static_cast<float>(w), static_cast<float>(h));
	if (style) e->setStyle(style);
	e->text = text;
	*c = e;
	return 1;
}
Box *checkLayoutBox(lua_State *L, int index)
{
	Box **c = static_cast<Box**>(luaL_checkudata(L, index, "layout"));
	luaL_argcheck(L, c != nullptr, index, "`layout' expected");
	return *c;
}
int pushLayoutBox(lua_State *L, Box *box)
{
	Box **c = static_cast<Box**>(lua_newuserdata(L, sizeof(Box*)));
	luaL_getmetatable(L, "layout");
	lua_setmetatable(L, -2);
	*c = static_cast<Box*>(box->ref());
	return 1;
}
int boxAdd(lua_State *L)
{
	Box *box = checkLayoutBox(L, 1);
	Box *box2 = checkLayoutBox(L, 2);
	box->addChild(static_cast<Box*>(box2->ref()));
	pushLayoutBox(L, box);
	return 1;
}
int boxHelperOnly(lua_State *L)
{
	Box *box = checkLayoutBox(L, 1);
	if (lua_type(L, 2) == LUA_TBOOLEAN){
		int v = lua_toboolean(L, 2);
		if (v){
			box->helper_only = true;
		}else{
			box->helper_only = false;
		}
		return 0;
	}else{
		lua_pushboolean(L, box->helper_only);
		return 1;
	}
}
int boxLocked(lua_State *L)
{
	Box *box = checkLayoutBox(L, 1);
	if (lua_type(L, 2) == LUA_TBOOLEAN){
		int v = lua_toboolean(L, 2);
		if (v){
			box->locked = true;
		}else{
			box->locked = false;
		}
		return 0;
	}else{
		lua_pushboolean(L, box->locked);
		return 1;
	}
}
int boxGc(lua_State *L) {
	Box *box = checkLayoutBox(L, 1);
	Box::unref(box);
	return 0;
}
System *checkLayoutSystem(lua_State *L, int index)
{
	System **c = static_cast<System**>(luaL_checkudata(L, index, "layoutSystem"));
	luaL_argcheck(L, c != nullptr, index, "`layoutSystem' expected");
	return *c;
}
int pushLayoutSystem(lua_State *L, System *system)
{
	System **c = static_cast<System**>(lua_newuserdata(L, sizeof(System*)));
	luaL_getmetatable(L, "layoutSystem");
	lua_setmetatable(L, -2);
	*c = system;
	return 1;
}
int systemAddStyle(lua_State *L)
{
	System *system = checkLayoutSystem(L, 1);
	Style *style = checkLayoutStyle(L, 2);
	system->addStyle(style);
	return 0;
}
int systemSetBox(lua_State *L)
{
	System *system = checkLayoutSystem(L, 1);
	Box *box = checkLayoutBox(L, 2);
	system->setBox(box);
	return 0;
}
static const struct luaL_Reg system_members[] =
{
	{"addStyle", systemAddStyle},
	{"setBox", systemSetBox},
	{nullptr, nullptr}
};
static const struct luaL_Reg box_members[] =
{
	{"add", boxAdd},
	{"helperOnly", boxHelperOnly},
	{"locked", boxLocked},
	{"__gc", boxGc},
	{nullptr, nullptr}
};
static const struct luaL_Reg style_members[] =
{
	{"label", styleLabel},
	{"__gc", styleGc},
	{nullptr, nullptr}
};
static const struct luaL_Reg functions[] =
{
	{"newBox", newLayoutBox},
	{"newText", newLayoutText},
	{"newFill", newLayoutFill},
	{"newStyle", newLayoutStyle},
	{nullptr, nullptr}
};
int registerLayout(lua_State *L)
{
	Script script(L);
	script.createType("layout", box_members);
	script.createType("layoutStyle", style_members);
	script.createType("layoutSystem", system_members);
	luaL_newlib(L, functions);
	return 1;
}
}
