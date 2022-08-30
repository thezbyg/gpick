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
#include "Lua.h"
#include "GlobalState.h"
#include "layout/Box.h"
#include "layout/Layout.h"
#include "layout/Style.h"
#include "layout/System.h"
#include <typeinfo>
#include <iostream>
namespace lua {
static int newLayoutStyle(lua_State *L) {
	const char *name = luaL_checkstring(L, 2);
	Color color = checkColor(L, 3);
	double fontSize = luaL_optnumber(L, 4, 1.0);
	layout::Style **c = static_cast<layout::Style **>(lua_newuserdata(L, sizeof(layout::Style *)));
	luaL_getmetatable(L, "layoutStyle");
	lua_setmetatable(L, -2);
	*c = new layout::Style(name, color, static_cast<float>(fontSize));
	return 1;
}
common::Ref<layout::Style> checkLayoutStyle(lua_State *L, int index) {
	layout::Style **c = static_cast<layout::Style **>(luaL_checkudata(L, index, "layoutStyle"));
	luaL_argcheck(L, c != nullptr, index, "`layoutStyle' expected");
	return common::Ref<layout::Style>((*c)->reference());
}
int pushLayoutStyle(lua_State *L, common::Ref<layout::Style> style) {
	layout::Style **c = static_cast<layout::Style **>(lua_newuserdata(L, sizeof(layout::Style *)));
	luaL_getmetatable(L, "layoutStyle");
	lua_setmetatable(L, -2);
	*c = style.unwrap();
	return 1;
}
int styleGc(lua_State *L) {
	checkLayoutStyle(L, 1)->release();
	return 0;
}
int styleLabel(lua_State *L) {
	common::Ref<layout::Style> style = checkLayoutStyle(L, 1);
	if (lua_type(L, 2) == LUA_TSTRING) {
		const char *name = luaL_checkstring(L, 2);
		style->setLabel(name);
		return 0;
	} else {
		lua_pushstring(L, style->label().c_str());
		return 1;
	}
}
static int newLayoutBox(lua_State *L) {
	const char *name = luaL_checkstring(L, 2);
	double x = luaL_checknumber(L, 3);
	double y = luaL_checknumber(L, 4);
	double w = luaL_checknumber(L, 5);
	double h = luaL_checknumber(L, 6);
	layout::Box **c = static_cast<layout::Box **>(lua_newuserdata(L, sizeof(layout::Box *)));
	luaL_getmetatable(L, "layout");
	lua_setmetatable(L, -2);
	*c = new layout::Box(name, static_cast<float>(x), static_cast<float>(y), static_cast<float>(w), static_cast<float>(h));
	return 1;
}
static int newLayoutFill(lua_State *L) {
	const char *name = luaL_checkstring(L, 2);
	double x = luaL_checknumber(L, 3);
	double y = luaL_checknumber(L, 4);
	double w = luaL_checknumber(L, 5);
	double h = luaL_checknumber(L, 6);
	common::Ref<layout::Style> style = checkLayoutStyle(L, 7);
	layout::Box **c = static_cast<layout::Box **>(lua_newuserdata(L, sizeof(layout::Box *)));
	luaL_getmetatable(L, "layout");
	lua_setmetatable(L, -2);
	layout::Fill *e = new layout::Fill(name, static_cast<float>(x), static_cast<float>(y), static_cast<float>(w), static_cast<float>(h));
	e->setStyle(style);
	*c = e;
	return 1;
}
static int newLayoutCircle(lua_State *L) {
	const char *name = luaL_checkstring(L, 2);
	double x = luaL_checknumber(L, 3);
	double y = luaL_checknumber(L, 4);
	double w = luaL_checknumber(L, 5);
	double h = luaL_checknumber(L, 6);
	common::Ref<layout::Style> style = checkLayoutStyle(L, 7);
	layout::Box **c = static_cast<layout::Box **>(lua_newuserdata(L, sizeof(layout::Box *)));
	luaL_getmetatable(L, "layout");
	lua_setmetatable(L, -2);
	layout::Circle *e = new layout::Circle(name, static_cast<float>(x), static_cast<float>(y), static_cast<float>(w), static_cast<float>(h));
	e->setStyle(style);
	*c = e;
	return 1;
}
static int newLayoutPie(lua_State *L) {
	const char *name = luaL_checkstring(L, 2);
	double x = luaL_checknumber(L, 3);
	double y = luaL_checknumber(L, 4);
	double w = luaL_checknumber(L, 5);
	double h = luaL_checknumber(L, 6);
	double start = luaL_checknumber(L, 7);
	double end = luaL_checknumber(L, 8);
	common::Ref<layout::Style> style = checkLayoutStyle(L, 9);
	layout::Box **c = static_cast<layout::Box **>(lua_newuserdata(L, sizeof(layout::Box *)));
	luaL_getmetatable(L, "layout");
	lua_setmetatable(L, -2);
	layout::Pie *e = new layout::Pie(name, static_cast<float>(x), static_cast<float>(y), static_cast<float>(w), static_cast<float>(h));
	e->setStyle(style);
	e->setStartAngle(static_cast<float>(start));
	e->setEndAngle(static_cast<float>(end));
	*c = e;
	return 1;
}
static int newLayoutText(lua_State *L) {
	const char *name = luaL_checkstring(L, 2);
	double x = luaL_checknumber(L, 3);
	double y = luaL_checknumber(L, 4);
	double w = luaL_checknumber(L, 5);
	double h = luaL_checknumber(L, 6);
	common::Ref<layout::Style> style;
	if (lua_type(L, 7) != LUA_TNIL) {
		style = checkLayoutStyle(L, 7);
	}
	const char *text = luaL_checkstring(L, 8);
	layout::Box **c = static_cast<layout::Box **>(lua_newuserdata(L, sizeof(layout::Box *)));
	luaL_getmetatable(L, "layout");
	lua_setmetatable(L, -2);
	layout::Text *e = new layout::Text(name, static_cast<float>(x), static_cast<float>(y), static_cast<float>(w), static_cast<float>(h));
	if (style)
		e->setStyle(style);
	e->setText(text);
	*c = e;
	return 1;
}
common::Ref<layout::Box> checkLayoutBox(lua_State *L, int index) {
	layout::Box **c = static_cast<layout::Box **>(luaL_checkudata(L, index, "layout"));
	luaL_argcheck(L, c != nullptr, index, "`layout' expected");
	return common::Ref<layout::Box>((*c)->reference());
}
int pushLayoutBox(lua_State *L, common::Ref<layout::Box> box) {
	layout::Box **c = static_cast<layout::Box **>(lua_newuserdata(L, sizeof(layout::Box *)));
	luaL_getmetatable(L, "layout");
	lua_setmetatable(L, -2);
	*c = box.unwrap();
	return 1;
}
int boxAdd(lua_State *L) {
	common::Ref<layout::Box> box = checkLayoutBox(L, 1);
	common::Ref<layout::Box> box2 = checkLayoutBox(L, 2);
	box->addChild(box2);
	pushLayoutBox(L, box);
	return 1;
}
int boxHelperOnly(lua_State *L) {
	common::Ref<layout::Box> box = checkLayoutBox(L, 1);
	if (lua_type(L, 2) == LUA_TBOOLEAN) {
		int v = lua_toboolean(L, 2);
		box->setHelperOnly(v);
		return 0;
	} else {
		lua_pushboolean(L, box->helperOnly());
		return 1;
	}
}
int boxLocked(lua_State *L) {
	common::Ref<layout::Box> box = checkLayoutBox(L, 1);
	if (lua_type(L, 2) == LUA_TBOOLEAN) {
		int v = lua_toboolean(L, 2);
		box->setLocked(v);
		return 0;
	} else {
		lua_pushboolean(L, box->locked());
		return 1;
	}
}
int boxGc(lua_State *L) {
	checkLayoutBox(L, 1)->release();
	return 0;
}
common::Ref<layout::System> checkLayoutSystem(lua_State *L, int index) {
	layout::System **c = static_cast<layout::System **>(luaL_checkudata(L, index, "layoutSystem"));
	luaL_argcheck(L, c != nullptr, index, "`layoutSystem' expected");
	return common::Ref<layout::System>((*c)->reference());
}
int pushLayoutSystem(lua_State *L, common::Ref<layout::System> system) {
	layout::System **c = static_cast<layout::System **>(lua_newuserdata(L, sizeof(layout::System *)));
	luaL_getmetatable(L, "layoutSystem");
	lua_setmetatable(L, -2);
	*c = system.unwrap();
	return 1;
}
int systemAddStyle(lua_State *L) {
	common::Ref<layout::System> system = checkLayoutSystem(L, 1);
	common::Ref<layout::Style> style = checkLayoutStyle(L, 2);
	system->addStyle(style);
	return 0;
}
int systemSetBox(lua_State *L) {
	common::Ref<layout::System> system = checkLayoutSystem(L, 1);
	common::Ref<layout::Box> box = checkLayoutBox(L, 2);
	system->setBox(box);
	return 0;
}
static const struct luaL_Reg systemMembers[] = {
	{ "addStyle", systemAddStyle },
	{ "setBox", systemSetBox },
	{ nullptr, nullptr }
};
static const struct luaL_Reg boxMembers[] = {
	{ "add", boxAdd },
	{ "helperOnly", boxHelperOnly },
	{ "locked", boxLocked },
	{ "__gc", boxGc },
	{ nullptr, nullptr }
};
static const struct luaL_Reg styleMembers[] = {
	{ "label", styleLabel },
	{ "__gc", styleGc },
	{ nullptr, nullptr }
};
static const struct luaL_Reg functions[] = {
	{ "newBox", newLayoutBox },
	{ "newText", newLayoutText },
	{ "newFill", newLayoutFill },
	{ "newCircle", newLayoutCircle },
	{ "newPie", newLayoutPie },
	{ "newStyle", newLayoutStyle },
	{ nullptr, nullptr }
};
int registerLayout(lua_State *L) {
	Script script(L);
	script.createType("layout", boxMembers);
	script.createType("layoutStyle", styleMembers);
	script.createType("layoutSystem", systemMembers);
	luaL_newlib(L, functions);
	return 1;
}
}
