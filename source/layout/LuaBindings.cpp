/*
 * Copyright (c) 2009, Albertas Vyšniauskas
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

#include <typeinfo>
#include <iostream>
using namespace std;

namespace layout{

static int lua_lstyle_new (lua_State *L) {
	Style** c = static_cast<Style**>(lua_newuserdata(L, sizeof(Style*)));
	luaL_getmetatable(L, "layout_style");
	lua_setmetatable(L, -2);
	
	size_t st;
	const char* name = luaL_checklstring(L, 2, &st);
	Color* background_color = lua_checkcolor(L, 3);
	Color* text_color = lua_checkcolor(L, 4);
	double font_size = luaL_checknumber(L, 5);
	
	Style *e = new Style(name, background_color, text_color, font_size);
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

static const struct luaL_reg lua_lstylelib_f [] = {
	{"new", lua_lstyle_new},
	{NULL, NULL}
};

static const struct luaL_reg lua_lstylelib_m [] = {
	{NULL, NULL}
};

	

static int lua_new_box (lua_State *L) {
	Box** c = (Box**)lua_newuserdata(L, sizeof(Box*));
	luaL_getmetatable(L, "layout");
	lua_setmetatable(L, -2);
	
	size_t st;
	const char* name = luaL_checklstring(L, 2, &st);
	double x = luaL_checknumber(L, 3);
	double y = luaL_checknumber(L, 4);
	double w = luaL_checknumber(L, 5);
	double h = luaL_checknumber(L, 6);
	
	Box *e = new Box(name, x, y, w, h);
	*c = static_cast<Box*>(e);
	return 1;
}

static int lua_new_fill (lua_State *L) {
	Box** c = (Box**)lua_newuserdata(L, sizeof(Box*));
	luaL_getmetatable(L, "layout");
	lua_setmetatable(L, -2);
	
	size_t st;
	const char* name = luaL_checklstring(L, 2, &st);
	double x = luaL_checknumber(L, 3);
	double y = luaL_checknumber(L, 4);
	double w = luaL_checknumber(L, 5);
	double h = luaL_checknumber(L, 6);
	
	Style* style = lua_checklstyle(L, 7);
	
	Fill *e = new Fill(name, x, y, w, h);
	e->SetStyle(style);
	*c = static_cast<Box*>(e);
	
	return 1;
}

static int lua_new_text (lua_State *L) {
	Box** c = (Box**)lua_newuserdata(L, sizeof(Box*));
	luaL_getmetatable(L, "layout");
	lua_setmetatable(L, -2);
	
	size_t st;
	const char* name = luaL_checklstring(L, 2, &st);
	double x = luaL_checknumber(L, 3);
	double y = luaL_checknumber(L, 4);
	double w = luaL_checknumber(L, 5);
	double h = luaL_checknumber(L, 6);
	
	Style* style = lua_checklstyle(L, 7);
	
	const char* text = luaL_checklstring(L, 8, &st);
	
	Text *e = new Text(name, x, y, w, h);
	e->SetStyle(style);
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


int lua_gclbox (lua_State *L) {
	Box* box = lua_checklbox(L, 1);
	Box::unref(box);
	return 0;
}

static const struct luaL_reg lua_lboxlib_f [] = {
	{"new_box", lua_new_box},
	{"new_text", lua_new_text},
	{"new_fill", lua_new_fill},
	{NULL, NULL}
};

static const struct luaL_reg lua_lboxlib_m [] = {
	{"add", lua_add},
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

static const struct luaL_reg lua_systemlib_m [] = {
	{"addstyle", lua_lsystem_addstyle},
	{"setbox", lua_lsystem_setbox},
	{NULL, NULL}
};


int luaopen_lbox (lua_State *L) {
	luaL_newmetatable(L, "layout");

	lua_pushstring(L, "__index");
	lua_pushvalue(L, -2);  /* pushes the metatable */
	lua_settable(L, -3);  /* metatable.__index = metatable */

	lua_pushstring(L, "__gc");
	lua_pushcfunction(L, lua_gclbox);
	lua_settable(L, -3);
	
	luaL_openlib(L, NULL, lua_lboxlib_m, 0);
	luaL_openlib(L, "layout", lua_lboxlib_f, 0);
	
	
	luaL_newmetatable(L, "layout_style");

	lua_pushstring(L, "__index");
	lua_pushvalue(L, -2);  /* pushes the metatable */
	lua_settable(L, -3);  /* metatable.__index = metatable */

	lua_pushstring(L, "__gc");
	lua_pushcfunction(L, lua_lstyle_gc);
	lua_settable(L, -3);
	
	luaL_openlib(L, NULL, lua_lstylelib_m, 0);
	luaL_openlib(L, "layout_style", lua_lstylelib_f, 0);
	
	
	
	luaL_newmetatable(L, "layout_system");

	lua_pushstring(L, "__index");
	lua_pushvalue(L, -2);  /* pushes the metatable */
	lua_settable(L, -3);  /* metatable.__index = metatable */

	luaL_openlib(L, NULL, lua_systemlib_m, 0);
		
	return 1;
}


int lua_ext_layout_openlib(lua_State *L){
    int status;
	
	lua_pushcfunction(L, luaopen_lbox);
    lua_pushstring(L, 0);
    status = lua_pcall(L, 1, 0, 0);
	if (status) {
		cerr<<"lua_ext_layout_openlib: "<<lua_tostring (L, -1)<<endl;
	}
    return 0;
}


}

