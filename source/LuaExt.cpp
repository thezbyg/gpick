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

#include "LuaExt.h"
#include <glib.h>
#include "Internationalisation.h"

#include <iostream>
using namespace std;

static int lua_newcolor (lua_State *L) {
	Color *c = (Color*)lua_newuserdata(L, sizeof(Color));
	luaL_getmetatable(L, "color");
	lua_setmetatable(L, -2);
	if (lua_type(L, 2)==LUA_TNUMBER && lua_type(L, 3)==LUA_TNUMBER  && lua_type(L, 4)==LUA_TNUMBER ){
		c->rgb.red = luaL_checknumber(L, 2);
		c->rgb.green = luaL_checknumber(L, 3);
		c->rgb.blue = luaL_checknumber(L, 4);
	}else{
		color_zero(c);
	}
	return 1;
}

Color *lua_checkcolor (lua_State *L, int index) {
	void *ud = luaL_checkudata(L, index, "color");
	luaL_argcheck(L, ud != NULL, index, "`color' expected");
	return (Color *)ud;
}

int lua_pushcolor (lua_State *L, const Color* color) {
	Color *c = (Color*)lua_newuserdata(L, sizeof(Color));
	luaL_getmetatable(L, "color");
	lua_setmetatable(L, -2);
	color_copy(color, c);
	return 1;
}

static int lua_color2string (lua_State *L) {
	Color *c = lua_checkcolor(L, 1);
	lua_pushfstring(L, "color(%f, %f, %f)", c->rgb.red, c->rgb.green, c->rgb.blue);
	return 1;
}

static int lua_color_rgb (lua_State *L) {
	Color *c = lua_checkcolor(L, 1);
	if (lua_type(L, 2)==LUA_TNUMBER && lua_type(L, 3)==LUA_TNUMBER  && lua_type(L, 4)==LUA_TNUMBER ){
		c->rgb.red = luaL_checknumber(L, 2);
		c->rgb.green = luaL_checknumber(L, 3);
		c->rgb.blue = luaL_checknumber(L, 4);
	}
	lua_pushnumber(L, c->rgb.red);
	lua_pushnumber(L, c->rgb.green);
	lua_pushnumber(L, c->rgb.blue);
	return 3;
}

static int lua_color_red (lua_State *L) {
	Color *c = lua_checkcolor(L, 1);
	if (lua_type(L, 2)==LUA_TNUMBER){
		c->rgb.red=luaL_checknumber(L, 2);
	}
	lua_pushnumber(L, c->rgb.red);
	return 1;
}

static int lua_color_green (lua_State *L) {
	Color *c = lua_checkcolor(L, 1);
	if (lua_type(L, 2)==LUA_TNUMBER){
		c->rgb.green=luaL_checknumber(L, 2);
	}
	lua_pushnumber(L, c->rgb.green);
	return 1;
}

static int lua_color_blue (lua_State *L) {
	Color *c = lua_checkcolor(L, 1);
	if (lua_type(L, 2)==LUA_TNUMBER){
		c->rgb.blue=luaL_checknumber(L, 2);
	}
	lua_pushnumber(L, c->rgb.blue);
	return 1;
}


static int lua_color_hsl (lua_State *L) {
	Color *c = lua_checkcolor(L, 1);
	if (lua_type(L, 2)==LUA_TNUMBER && lua_type(L, 3)==LUA_TNUMBER  && lua_type(L, 4)==LUA_TNUMBER ){
		c->hsl.hue = luaL_checknumber(L, 2);
		c->hsl.saturation = luaL_checknumber(L, 3);
		c->hsl.lightness = luaL_checknumber(L, 4);
	}
	lua_pushnumber(L, c->hsl.hue);
	lua_pushnumber(L, c->hsl.saturation);
	lua_pushnumber(L, c->hsl.lightness);
	return 3;
}

static int lua_color_hue (lua_State *L) {
	Color *c = lua_checkcolor(L, 1);
	if (lua_type(L, 2)==LUA_TNUMBER){
		c->hsl.hue=luaL_checknumber(L, 2);
	}
	lua_pushnumber(L, c->hsl.hue);
	return 1;
}

static int lua_color_saturation (lua_State *L) {
	Color *c = lua_checkcolor(L, 1);
	if (lua_type(L, 2)==LUA_TNUMBER){
		c->hsl.saturation=luaL_checknumber(L, 2);
	}
	lua_pushnumber(L, c->hsl.saturation);
	return 1;
}

static int lua_color_lightness (lua_State *L) {
	Color *c = lua_checkcolor(L, 1);
	if (lua_type(L, 2)==LUA_TNUMBER){
		c->hsl.lightness=luaL_checknumber(L, 2);
	}
	lua_pushnumber(L, c->hsl.lightness);
	return 1;
}


static int lua_color_cmyk (lua_State *L) {
	Color *c = lua_checkcolor(L, 1);
	if (lua_type(L, 2)==LUA_TNUMBER && lua_type(L, 3)==LUA_TNUMBER  && lua_type(L, 4)==LUA_TNUMBER  && lua_type(L, 5)==LUA_TNUMBER ){
		c->cmyk.c = luaL_checknumber(L, 2);
		c->cmyk.m = luaL_checknumber(L, 3);
		c->cmyk.y = luaL_checknumber(L, 4);
		c->cmyk.k = luaL_checknumber(L, 5);
	}
	lua_pushnumber(L, c->cmyk.c);
	lua_pushnumber(L, c->cmyk.m);
	lua_pushnumber(L, c->cmyk.y);
	lua_pushnumber(L, c->cmyk.k);
	return 4;
}

static int lua_color_cyan (lua_State *L) {
	Color *c = lua_checkcolor(L, 1);
	if (lua_type(L, 2)==LUA_TNUMBER){
		c->cmyk.c = luaL_checknumber(L, 2);
	}
	lua_pushnumber(L, c->cmyk.c);
	return 1;
}

static int lua_color_magenta (lua_State *L) {
	Color *c = lua_checkcolor(L, 1);
	if (lua_type(L, 2)==LUA_TNUMBER){
		c->cmyk.m = luaL_checknumber(L, 2);
	}
	lua_pushnumber(L, c->cmyk.m);
	return 1;
}

static int lua_color_yellow (lua_State *L) {
	Color *c = lua_checkcolor(L, 1);
	if (lua_type(L, 2)==LUA_TNUMBER){
		c->cmyk.y = luaL_checknumber(L, 2);
	}
	lua_pushnumber(L, c->cmyk.y);
	return 1;
}

static int lua_color_key_black (lua_State *L) {
	Color *c = lua_checkcolor(L, 1);
	if (lua_type(L, 2)==LUA_TNUMBER){
		c->cmyk.k = luaL_checknumber(L, 2);
	}
	lua_pushnumber(L, c->cmyk.k);
	return 1;
}


static int lua_color_lab_lightness (lua_State *L) {
	Color *c = lua_checkcolor(L, 1);
	if (lua_type(L, 2)==LUA_TNUMBER){
		c->lab.L = luaL_checknumber(L, 2);
	}
	lua_pushnumber(L, c->lab.L);
	return 1;
}

static int lua_color_lab_a (lua_State *L) {
	Color *c = lua_checkcolor(L, 1);
	if (lua_type(L, 2)==LUA_TNUMBER){
		c->lab.a = luaL_checknumber(L, 2);
	}
	lua_pushnumber(L, c->lab.a);
	return 1;
}

static int lua_color_lab_b (lua_State *L) {
	Color *c = lua_checkcolor(L, 1);
	if (lua_type(L, 2)==LUA_TNUMBER){
		c->lab.b = luaL_checknumber(L, 2);
	}
	lua_pushnumber(L, c->lab.b);
	return 1;
}


static int lua_color_rgb_to_hsl (lua_State *L) {
	Color *c = lua_checkcolor(L, 1);
	Color c2;
	color_rgb_to_hsl(c, &c2);
	lua_pushcolor(L, &c2);
	return 1;
}

static int lua_color_hsl_to_rgb (lua_State *L) {
	Color *c = lua_checkcolor(L, 1);
	Color c2;
	color_hsl_to_rgb(c, &c2);
	lua_pushcolor(L, &c2);
	return 1;
}


static int lua_color_rgb_to_cmyk (lua_State *L) {
	Color *c = lua_checkcolor(L, 1);
	Color c2, c3;
	color_rgb_to_cmy(c, &c3);
	color_cmy_to_cmyk(&c3, &c2);
	lua_pushcolor(L, &c2);
	return 1;
}

static int lua_color_lch_lightness(lua_State *L) {
	Color *c = lua_checkcolor(L, 1);
	if (lua_type(L, 2)==LUA_TNUMBER){
		c->lch.L = luaL_checknumber(L, 2);
	}
	lua_pushnumber(L, c->lch.L);
	return 1;
}

static int lua_color_lch_chroma(lua_State *L) {
	Color *c = lua_checkcolor(L, 1);
	if (lua_type(L, 2)==LUA_TNUMBER){
		c->lch.C = luaL_checknumber(L, 2);
	}
	lua_pushnumber(L, c->lch.C);
	return 1;
}

static int lua_color_lch_hue(lua_State *L) {
	Color *c = lua_checkcolor(L, 1);
	if (lua_type(L, 2)==LUA_TNUMBER){
		c->lch.h = luaL_checknumber(L, 2);
	}
	lua_pushnumber(L, c->lch.h);
	return 1;
}

static const struct luaL_Reg lua_colorlib_f [] = {
	{"new", lua_newcolor},
	{NULL, NULL}
};

static const struct luaL_Reg lua_colorlib_m [] = {
	{"__tostring", lua_color2string},
	{"red",			lua_color_red},
	{"green",		lua_color_green},
	{"blue",		lua_color_blue},
	{"rgb",			lua_color_rgb},

	{"hue",			lua_color_hue},
	{"saturation",	lua_color_saturation},
	{"lightness",	lua_color_lightness},
	{"value",	lua_color_lightness},
	{"hsl",			lua_color_hsl},

	{"cyan",		lua_color_cyan},
	{"magenta",		lua_color_magenta},
	{"yellow",		lua_color_yellow},
	{"key_black",	lua_color_key_black},
	{"cmyk",		lua_color_cmyk},

	{"lab_lightness",		lua_color_lab_lightness},
	{"lab_a",		lua_color_lab_a},
	{"lab_b",		lua_color_lab_b},

	{"lch_lightness",		lua_color_lch_lightness},
	{"lch_chroma",		lua_color_lch_chroma},
	{"lch_hue",		lua_color_lch_hue},

	{"rgb_to_hsl",	lua_color_rgb_to_hsl},
	{"hsl_to_rgb",	lua_color_hsl_to_rgb},
	{"rgb_to_cmyk",	lua_color_rgb_to_cmyk},
	{NULL, NULL}
};

static int luaopen_color(lua_State *L) {
	luaL_newmetatable(L, "color");
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	luaL_setfuncs(L, lua_colorlib_m, 0);
	lua_pop(L, 1);

	luaL_newlibtable(L, lua_colorlib_f);
	luaL_setfuncs(L, lua_colorlib_f, 0);
	lua_setglobal(L, "color");
	return 1;
}

static int lua_newcolorobject (lua_State *L) {
	struct ColorObject** c = (struct ColorObject**)lua_newuserdata(L, sizeof(struct ColorObject*));
	luaL_getmetatable(L, "colorobject");
	lua_setmetatable(L, -2);
	*c=NULL;
	return 1;
}

struct ColorObject** lua_checkcolorobject (lua_State *L, int index) {
	void *ud = luaL_checkudata(L, index, "colorobject");
	luaL_argcheck(L, ud != NULL, index, "`colorobject' expected");
	return (struct ColorObject **)ud;
}

int lua_pushcolorobject (lua_State *L, struct ColorObject* color_object) {
	struct ColorObject** c = (struct ColorObject**)lua_newuserdata(L, sizeof(struct ColorObject*));
	luaL_getmetatable(L, "colorobject");
	lua_setmetatable(L, -2);
	*c=color_object;
	return 1;
}

int lua_colorobject_get_color(lua_State *L) {
	struct ColorObject** color_object=lua_checkcolorobject(L, 1);
	Color tmp;
	color_object_get_color(*color_object, &tmp);
	lua_pushcolor(L, &tmp);
	return 1;
}

int lua_colorobject_set_color(lua_State *L) {
	struct ColorObject** color_object=lua_checkcolorobject(L, 1);
	Color *c = lua_checkcolor(L, 2);
	color_object_set_color(*color_object, c);
	return 0;
}

static const struct luaL_Reg lua_colorobjectlib_f [] = {
	{"new", lua_newcolorobject},
	{NULL, NULL}
};

static const struct luaL_Reg lua_colorobjectlib_m [] = {
	{"get_color", lua_colorobject_get_color},
	{"set_color", lua_colorobject_set_color},
	{NULL, NULL}
};


int luaopen_colorobject (lua_State *L) {
	luaL_newmetatable(L, "colorobject");
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	luaL_setfuncs(L, lua_colorobjectlib_m, 0);
	lua_pop(L, 1);

	luaL_newlibtable(L, lua_colorobjectlib_f);
	luaL_setfuncs(L, lua_colorobjectlib_f, 0);
	lua_setglobal(L, "colorobject");
	return 1;
}

int lua_i18n_gettext(lua_State *L)
{
  const char *text = luaL_checkstring(L, 1);
	lua_pushstring(L, _(text));
	return 1;
}

int luaopen_i18n(lua_State *L)
{
	lua_pushcclosure(L, lua_i18n_gettext, 0);
	lua_setglobal(L, "_");
	return 1;
}


int lua_ext_colors_openlib(lua_State *L){
	luaopen_color(L);
	luaopen_colorobject(L);
	luaopen_i18n(L);
	return 0;
}

