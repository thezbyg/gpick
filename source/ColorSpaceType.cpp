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

const ColorSpaceType colorSpaceTypes[] = {
	{ColorSpace::hsv, "hsv", 4,
		{
			{_("Hue"), "H", 360, 0, 360, 0.01},
			{_("Saturation"), "S", 100, 0, 100, 0.01},
			{_("Value"), "V", 100, 0, 100, 0.01},
			{_("Alpha"), "A", 100, 0, 100, 0.01},
		},
	},
	{ColorSpace::hsl, "hsl", 4,
		{
			{_("Hue"), "H", 360, 0, 360, 0.01},
			{_("Saturation"), "S", 100, 0, 100, 0.01},
			{_("Lightness"), "L", 100, 0, 100, 0.01},
			{_("Alpha"), "A", 100, 0, 100, 0.01},
		},
	},
	{ColorSpace::rgb, "rgb", 4,
		{
			{_("Red"), "R", 255, 0, 255, 0.01},
			{_("Green"), "G", 255, 0, 255, 0.01},
			{_("Blue"), "B", 255, 0, 255, 0.01},
			{_("Alpha"), "A", 100, 0, 100, 0.01},
		},
	},
	{ColorSpace::cmyk, "cmyk", 5,
		{
			{_("Cyan"), "C", 255, 0, 255, 0.01},
			{_("Magenta"), "M", 255, 0, 255, 0.01},
			{_("Yellow"), "Y", 255, 0, 255, 0.01},
			{_("Key"), "K", 255, 0, 255, 0.01},
			{_("Alpha"), "A", 100, 0, 100, 0.01},
		}
	},
	{ColorSpace::lab, "lab", 4,
		{
			{_("Lightness"), "L", 1, 0, 100, 0.0001},
			{"a", "a", 1, -145, 145, 0.0001},
			{"b", "b", 1, -145, 145, 0.0001},
			{_("Alpha"), "A", 100, 0, 100, 0.01},
		}
	},
	{ColorSpace::lch, "lch", 4,
		{
			{_("Lightness"), "L", 1, 0, 100, 0.0001},
			{_("Chroma"), "C", 1, 0, 100, 0.0001},
			{_("Hue"), "H", 1, 0, 360, 0.0001},
			{_("Alpha"), "A", 100, 0, 100, 0.01},
		}
	},
};
const ColorSpaceType *color_space_get_types() {
	return colorSpaceTypes;
}
size_t color_space_count_types() {
	return sizeof(colorSpaceTypes) / sizeof(ColorSpaceType);
}
const ColorSpaceType *color_space_get(ColorSpace colorSpace) {
	for (size_t i = 0; i < sizeof(colorSpaceTypes) / sizeof(ColorSpaceType); ++i) {
		if (colorSpaceTypes[i].colorSpace == colorSpace)
			return &colorSpaceTypes[i];
	}
	return nullptr;
}
std::vector<std::string> color_space_color_to_text(const char *type, const Color &color, float alpha, lua::Script &script, GlobalState *gs) {
	std::vector<std::string> result;
	if (!gs->callbacks().componentToText().valid())
		return result;
	lua_State *L = script;
	int stackTop = lua_gettop(L);
	gs->callbacks().componentToText().get();
	lua_pushstring(L, type);
	lua::pushColor(L, color);
	lua_pushnumber(L, alpha);
	int status = lua_pcall(L, 3, 1, 0);
	if (status == 0) {
		if (lua_type(L, -1) == LUA_TTABLE) {
			for (int i = 0; i < 5; i++) {
				lua_pushinteger(L, i + 1);
				lua_gettable(L, -2);
				if (lua_type(L, -1) == LUA_TSTRING) {
					const char *converted = lua_tostring(L, -1);
					result.push_back(std::string(converted));
				}
				lua_pop(L, 1);
			}
			lua_settop(L, stackTop);
			return result;
		} else {
			std::cerr << "componentToText: returned not a table value, type is \"" << type << "\"" << '\n';
		}
	} else {
		std::cerr << "componentToText: " << lua_tostring(L, -1) << '\n';
	}
	lua_settop(L, stackTop);
	return result;
}
