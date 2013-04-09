/*
 * Copyright (c) 2009-2012, Albertas Vyšniauskas
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

#include "LuaBindings.h"
#include "Box.h"
#include "System.h"

extern "C"{
#include <lualib.h>
#include <lauxlib.h>
}

#include <typeinfo>
#include <iostream>
using namespace std;

namespace layout{

static int lua_lstyle_new (lua_State *L) {

	size_t st;
	const char* name = luaL_checklstring(L, 2, &st);
	Color* color = lua_checkcolor(L, 3);
	double font_size = luaL_optnumber(L, 4, 1.0);

	Style** c = static_cast<Style**>(lua_newuserdata(L, sizeof(Style*)));
	luaL_getmetatable(L, "layout_style");
	lua_setmetatable(L, -2);

	Style *e = new Style(name, color, font_size);
	*c = static_cast<Style*>(e);

	return 1;
}

Style* lua_checklstyle (lua_State *L, int index) {
	Style** c = static_cast<Style**>(luaL_checkudata(L, index, "layout_style"));
	luaL_argcheck(L, c != NULL, index, "`layout_style' expected");
	return *c;
}

int lua_pushlstyle (lua_State *L, Style* style) {
	Style** c = static_cast<Style**>(lua_newuserdata(L, sizeof(Style*)));
	luaL_getmetatable(L, "layout_style");
	lua_setmetatable(L, -2);
	*c = style;
	return 1;
}

int lua_lstyle_gc (lua_State *L) {
	Style* style = lua_checklstyle(L, 1);
	Style::unref(style);
	return 0;
}

int lua_lstyle_humanname (lua_State *L) {
	Style *style = lua_checklstyle(L, 1);
	if (lua_type(L, 2)==LUA_TSTRING){
		size_t st;
		const char* name = luaL_checklstring(L, 2, &st);
		style->human_name = name;
		return 0;
	}else{
		lua_pushstring(L, style->human_name.c_str());
		return 1;
	}
}


static const struct luaL_Reg lua_lstylelib_f [] = {
	{"new", lua_lstyle_new},
	{NULL, NULL}
};

static const struct luaL_Reg lua_lstylelib_m [] = {
	{"humanname", lua_lstyle_humanname},
	{"__gc", lua_lstyle_gc},
	{NULL, NULL}
};



static int lua_new_box (lua_State *L) {

	size_t st;
	const char* name = luaL_checklstring(L, 2, &st);
	double x = luaL_checknumber(L, 3);
	double y = luaL_checknumber(L, 4);
	double w = luaL_checknumber(L, 5);
	double h = luaL_checknumber(L, 6);

	Box** c = (Box**)lua_newuserdata(L, sizeof(Box*));
	luaL_getmetatable(L, "layout");
	lua_setmetatable(L, -2);

	Box *e = new Box(name, x, y, w, h);
	*c = static_cast<Box*>(e);
	return 1;
}

static int lua_new_fill (lua_State *L) {
	size_t st;
	const char* name = luaL_checklstring(L, 2, &st);
	double x = luaL_checknumber(L, 3);
	double y = luaL_checknumber(L, 4);
	double w = luaL_checknumber(L, 5);
	double h = luaL_checknumber(L, 6);

	Style* style = lua_checklstyle(L, 7);

	Box** c = (Box**)lua_newuserdata(L, sizeof(Box*));
	luaL_getmetatable(L, "layout");
	lua_setmetatable(L, -2);

	Fill *e = new Fill(name, x, y, w, h);
	e->SetStyle(style);
	*c = static_cast<Box*>(e);

	return 1;
}

static int lua_new_text (lua_State *L) {

	size_t st;
	const char* name = luaL_checklstring(L, 2, &st);
	double x = luaL_checknumber(L, 3);
	double y = luaL_checknumber(L, 4);
	double w = luaL_checknumber(L, 5);
	double h = luaL_checknumber(L, 6);

	Style* style = 0;
	if (lua_type(L, 7)!=LUA_TNIL){
		style = lua_checklstyle(L, 7);
	}
	const char* text = luaL_checklstring(L, 8, &st);

	Box** c = (Box**)lua_newuserdata(L, sizeof(Box*));
	luaL_getmetatable(L, "layout");
	lua_setmetatable(L, -2);

	Text *e = new Text(name, x, y, w, h);
	if (style) e->SetStyle(style);
	e->text = text;
	*c = static_cast<Box*>(e);

	return 1;
}

Box* lua_checklbox (lua_State *L, int index) {
	Box** c = static_cast<Box**>(luaL_checkudata(L, index, "layout"));
	luaL_argcheck(L, c != NULL, index, "`layout' expected");
	return *c;
}

int lua_pushlbox (lua_State *L, Box* box) {
	Box** c = static_cast<Box**>(lua_newuserdata(L, sizeof(Box*)));
	luaL_getmetatable(L, "layout");
	lua_setmetatable(L, -2);
	*c = static_cast<Box*>(box->ref());
	return 1;
}

int lua_add (lua_State *L) {
	Box* box = lua_checklbox(L, 1);
	Box* box2 = lua_checklbox(L, 2);
	box->AddChild(static_cast<Box*>(box2->ref()));
	lua_pushlbox(L, box);
	return 1;
}

int lua_box_helper_only (lua_State *L) {
	Box* box = lua_checklbox(L, 1);
	if (lua_type(L, 2)==LUA_TBOOLEAN){
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

int lua_box_locked (lua_State *L) {
	Box* box = lua_checklbox(L, 1);
	if (lua_type(L, 2)==LUA_TBOOLEAN){
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

int lua_box_gc(lua_State *L) {
	Box* box = lua_checklbox(L, 1);
	Box::unref(box);
	return 0;
}

static const struct luaL_Reg lua_lboxlib_f [] = {
	{"new_box", lua_new_box},
	{"new_text", lua_new_text},
	{"new_fill", lua_new_fill},
	{NULL, NULL}
};

static const struct luaL_Reg lua_lboxlib_m [] = {
	{"add", lua_add},
	{"helper_only", lua_box_helper_only},
	{"locked", lua_box_locked},
	{"__gc", lua_box_gc},
	{NULL, NULL}
};




System* lua_checklsystem (lua_State *L, int index) {
	System** c = static_cast<System**>(luaL_checkudata(L, index, "layout_system"));
	luaL_argcheck(L, c != NULL, index, "`layout_system' expected");
	return *c;
}

int lua_pushlsystem (lua_State *L, System* system) {
	System** c = static_cast<System**>(lua_newuserdata(L, sizeof(System*)));
	luaL_getmetatable(L, "layout_system");
	lua_setmetatable(L, -2);
	*c = static_cast<System*>(system);
	return 1;
}


int lua_lsystem_addstyle (lua_State *L) {
	System* system = lua_checklsystem(L, 1);
	Style* style = lua_checklstyle(L, 2);
	system->AddStyle(style);
	return 0;
}

int lua_lsystem_setbox (lua_State *L) {
	System* system = lua_checklsystem(L, 1);
	Box* box = lua_checklbox(L, 2);
	system->SetBox(box);
	return 0;
}

static const struct luaL_Reg lua_systemlib_m [] = {
	{"addstyle", lua_lsystem_addstyle},
	{"setbox", lua_lsystem_setbox},
	{NULL, NULL}
};


int luaopen_lbox (lua_State *L) {
	luaL_newmetatable(L, "layout");
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	luaL_setfuncs(L, lua_lboxlib_m, 0);
	lua_pop(L, 1);

	luaL_newlibtable(L, lua_lboxlib_f);
	luaL_setfuncs(L, lua_lboxlib_f, 0);
	lua_setglobal(L, "layout");

	luaL_newmetatable(L, "layout_style");
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	luaL_setfuncs(L, lua_lstylelib_m, 0);
	lua_pop(L, 1);

	luaL_newlibtable(L, lua_lstylelib_f);
	luaL_setfuncs(L, lua_lstylelib_f, 0);
	lua_setglobal(L, "layout_style");

	luaL_newmetatable(L, "layout_system");
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	luaL_setfuncs(L, lua_systemlib_m, 0);
	lua_pop(L, 1);

	return 1;
}


int lua_ext_layout_openlib(lua_State *L){
	luaopen_lbox(L);
	return 0;
}


}

