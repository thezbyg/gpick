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

#include "ColorSpaceType.h"
#include "I18N.h"
#include "GlobalState.h"
#include "lua/Color.h"
#include "lua/Script.h"
#include "lua/Callbacks.h"
#include <lua.h>
#include <iostream>
using namespace std;

const ColorSpaceType color_space_types[] = {
	{GtkColorComponentComp::hsv, 4,
		{
			{_("Hue"), 360, 0, 360, 0.01},
			{_("Saturation"), 100, 0, 100, 0.01},
			{_("Value"), 100, 0, 100, 0.01},
			{_("Alpha"), 100, 0, 100, 0.01},
		},
	},
	{GtkColorComponentComp::hsl, 4,
		{
			{_("Hue"), 360, 0, 360, 0.01},
			{_("Saturation"), 100, 0, 100, 0.01},
			{_("Lightness"), 100, 0, 100, 0.01},
			{_("Alpha"), 100, 0, 100, 0.01},
		},
	},
	{GtkColorComponentComp::rgb, 4,
		{
			{_("Red"), 255, 0, 255, 0.01},
			{_("Green"), 255, 0, 255, 0.01},
			{_("Blue"), 255, 0, 255, 0.01},
			{_("Alpha"), 100, 0, 100, 0.01},
		},
	},
	{GtkColorComponentComp::cmyk, 5,
		{
			{_("Cyan"), 255, 0, 255, 0.01},
			{_("Magenta"), 255, 0, 255, 0.01},
			{_("Yellow"), 255, 0, 255, 0.01},
			{_("Key"), 255, 0, 255, 0.01},
			{_("Alpha"), 100, 0, 100, 0.01},
		}
	},
	{GtkColorComponentComp::lab, 4,
		{
			{_("Lightness"), 1, 0, 100, 0.0001},
			{"a", 1, -145, 145, 0.0001},
			{"b", 1, -145, 145, 0.0001},
			{_("Alpha"), 100, 0, 100, 0.01},
		}
	},
	{GtkColorComponentComp::lch, 4,
		{
			{_("Lightness"), 1, 0, 100, 0.0001},
			{_("Chroma"), 1, 0, 100, 0.0001},
			{_("Hue"), 1, 0, 360, 0.0001},
			{_("Alpha"), 100, 0, 100, 0.01},
		}
	},
};
const ColorSpaceType *color_space_get_types()
{
	return color_space_types;
}
size_t color_space_count_types()
{
	return sizeof(color_space_types) / sizeof(ColorSpaceType);
}
std::vector<std::string> color_space_color_to_text(const char *type, const Color &color, float alpha, lua::Script &script, GlobalState *gs)
{
	vector<string> result;
	if (!gs->callbacks().componentToText().valid())
		return result;
	lua_State *L = script;
	int stack_top = lua_gettop(L);
	gs->callbacks().componentToText().get();
	lua_pushstring(L, type);
	lua::pushColor(L, color);
	lua_pushnumber(L, alpha);
	int status = lua_pcall(L, 3, 1, 0);
	if (status == 0){
		if (lua_type(L, -1) == LUA_TTABLE){
			for (int i = 0; i < 5; i++){
				lua_pushinteger(L, i + 1);
				lua_gettable(L, -2);
				if (lua_type(L, -1) == LUA_TSTRING){
					const char* converted = lua_tostring(L, -1);
					result.push_back(string(converted));
				}
				lua_pop(L, 1);
			}
			lua_settop(L, stack_top);
			return result;
		}else{
			cerr << "componentToText: returned not a table value, type is \"" << type << "\"" << endl;
		}
	}else{
		cerr << "componentToText: " << lua_tostring(L, -1) << endl;
	}
	lua_settop(L, stack_top);
	return result;
}
