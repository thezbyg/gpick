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

#include "Color.h"
#include "Lua.h"
#include "../Color.h"
namespace lua {
static int newColor(lua_State *L) {
	Color *c = reinterpret_cast<Color *>(lua_newuserdata(L, sizeof(Color)));
	luaL_getmetatable(L, "color");
	lua_setmetatable(L, -2);
	if (lua_type(L, 2) == LUA_TNUMBER && lua_type(L, 3) == LUA_TNUMBER && lua_type(L, 4) == LUA_TNUMBER) {
		c->red = static_cast<float>(luaL_checknumber(L, 2));
		c->green = static_cast<float>(luaL_checknumber(L, 3));
		c->blue = static_cast<float>(luaL_checknumber(L, 4));
		if (lua_type(L, 5) == LUA_TNUMBER) {
			c->alpha = static_cast<float>(luaL_checknumber(L, 5));
		} else {
			c->alpha = 1.0f;
		}
	} else {
		*c = { 0.0f, 0.0f, 0.0f, 1.0f };
	}
	return 1;
}
Color &checkColor(lua_State *L, int index) {
	void *ud = luaL_checkudata(L, index, "color");
	luaL_argcheck(L, ud != nullptr, index, "`color' expected");
	return *reinterpret_cast<Color *>(ud);
}
int pushColor(lua_State *L, const Color &color) {
	Color *c = reinterpret_cast<Color *>(lua_newuserdata(L, sizeof(Color)));
	luaL_getmetatable(L, "color");
	lua_setmetatable(L, -2);
	*c = color;
	return 1;
}
static int toString(lua_State *L) {
	Color &c = checkColor(L, 1);
	lua_pushfstring(L, "color(%f, %f, %f, %f)", c.red, c.green, c.blue, c.alpha);
	return 1;
}
static int colorRgb(lua_State *L) {
	Color &c = checkColor(L, 1);
	if (lua_type(L, 2) == LUA_TNUMBER && lua_type(L, 3) == LUA_TNUMBER && lua_type(L, 4) == LUA_TNUMBER) {
		c.red = static_cast<float>(luaL_checknumber(L, 2));
		c.green = static_cast<float>(luaL_checknumber(L, 3));
		c.blue = static_cast<float>(luaL_checknumber(L, 4));
	}
	lua_pushnumber(L, c.red);
	lua_pushnumber(L, c.green);
	lua_pushnumber(L, c.blue);
	return 3;
}
static int colorRgba(lua_State *L) {
	Color &c = checkColor(L, 1);
	if (lua_type(L, 2) == LUA_TNUMBER && lua_type(L, 3) == LUA_TNUMBER && lua_type(L, 4) == LUA_TNUMBER && lua_type(L, 5) == LUA_TNUMBER) {
		c.red = static_cast<float>(luaL_checknumber(L, 2));
		c.green = static_cast<float>(luaL_checknumber(L, 3));
		c.blue = static_cast<float>(luaL_checknumber(L, 4));
		c.alpha = static_cast<float>(luaL_checknumber(L, 5));
	}
	lua_pushnumber(L, c.red);
	lua_pushnumber(L, c.green);
	lua_pushnumber(L, c.blue);
	lua_pushnumber(L, c.alpha);
	return 4;
}
static int colorRed(lua_State *L) {
	Color &c = checkColor(L, 1);
	if (lua_type(L, 2) == LUA_TNUMBER) {
		c.red = static_cast<float>(luaL_checknumber(L, 2));
	}
	lua_pushnumber(L, c.red);
	return 1;
}
static int colorGreen(lua_State *L) {
	Color &c = checkColor(L, 1);
	if (lua_type(L, 2) == LUA_TNUMBER) {
		c.green = static_cast<float>(luaL_checknumber(L, 2));
	}
	lua_pushnumber(L, c.green);
	return 1;
}
static int colorBlue(lua_State *L) {
	Color &c = checkColor(L, 1);
	if (lua_type(L, 2) == LUA_TNUMBER) {
		c.blue = static_cast<float>(luaL_checknumber(L, 2));
	}
	lua_pushnumber(L, c.blue);
	return 1;
}
static int colorAlpha(lua_State *L) {
	Color &c = checkColor(L, 1);
	if (lua_type(L, 2) == LUA_TNUMBER) {
		c.alpha = static_cast<float>(luaL_checknumber(L, 2));
	}
	lua_pushnumber(L, c.alpha);
	return 1;
}
static int colorHsl(lua_State *L) {
	Color &c = checkColor(L, 1);
	if (lua_type(L, 2) == LUA_TNUMBER && lua_type(L, 3) == LUA_TNUMBER && lua_type(L, 4) == LUA_TNUMBER) {
		c.hsl.hue = static_cast<float>(luaL_checknumber(L, 2));
		c.hsl.saturation = static_cast<float>(luaL_checknumber(L, 3));
		c.hsl.lightness = static_cast<float>(luaL_checknumber(L, 4));
	}
	lua_pushnumber(L, c.hsl.hue);
	lua_pushnumber(L, c.hsl.saturation);
	lua_pushnumber(L, c.hsl.lightness);
	return 3;
}
static int colorHsla(lua_State *L) {
	Color &c = checkColor(L, 1);
	if (lua_type(L, 2) == LUA_TNUMBER && lua_type(L, 3) == LUA_TNUMBER && lua_type(L, 4) == LUA_TNUMBER && lua_type(L, 5) == LUA_TNUMBER) {
		c.hsl.hue = static_cast<float>(luaL_checknumber(L, 2));
		c.hsl.saturation = static_cast<float>(luaL_checknumber(L, 3));
		c.hsl.lightness = static_cast<float>(luaL_checknumber(L, 4));
		c.alpha = static_cast<float>(luaL_checknumber(L, 5));
	}
	lua_pushnumber(L, c.hsl.hue);
	lua_pushnumber(L, c.hsl.saturation);
	lua_pushnumber(L, c.hsl.lightness);
	lua_pushnumber(L, c.alpha);
	return 4;
}
static int colorHue(lua_State *L) {
	Color &c = checkColor(L, 1);
	if (lua_type(L, 2) == LUA_TNUMBER) {
		c.hsl.hue = static_cast<float>(luaL_checknumber(L, 2));
	}
	lua_pushnumber(L, c.hsl.hue);
	return 1;
}
static int colorSaturation(lua_State *L) {
	Color &c = checkColor(L, 1);
	if (lua_type(L, 2) == LUA_TNUMBER) {
		c.hsl.saturation = static_cast<float>(luaL_checknumber(L, 2));
	}
	lua_pushnumber(L, c.hsl.saturation);
	return 1;
}
static int colorLightness(lua_State *L) {
	Color &c = checkColor(L, 1);
	if (lua_type(L, 2) == LUA_TNUMBER) {
		c.hsl.lightness = static_cast<float>(luaL_checknumber(L, 2));
	}
	lua_pushnumber(L, c.hsl.lightness);
	return 1;
}
static int colorCmyk(lua_State *L) {
	Color &c = checkColor(L, 1);
	if (lua_type(L, 2) == LUA_TNUMBER && lua_type(L, 3) == LUA_TNUMBER && lua_type(L, 4) == LUA_TNUMBER && lua_type(L, 5) == LUA_TNUMBER) {
		c.cmyk.c = static_cast<float>(luaL_checknumber(L, 2));
		c.cmyk.m = static_cast<float>(luaL_checknumber(L, 3));
		c.cmyk.y = static_cast<float>(luaL_checknumber(L, 4));
		c.cmyk.k = static_cast<float>(luaL_checknumber(L, 5));
	}
	lua_pushnumber(L, c.cmyk.c);
	lua_pushnumber(L, c.cmyk.m);
	lua_pushnumber(L, c.cmyk.y);
	lua_pushnumber(L, c.cmyk.k);
	return 4;
}
static int colorCyan(lua_State *L) {
	Color &c = checkColor(L, 1);
	if (lua_type(L, 2) == LUA_TNUMBER) {
		c.cmyk.c = static_cast<float>(luaL_checknumber(L, 2));
	}
	lua_pushnumber(L, c.cmyk.c);
	return 1;
}
static int colorMagenta(lua_State *L) {
	Color &c = checkColor(L, 1);
	if (lua_type(L, 2) == LUA_TNUMBER) {
		c.cmyk.m = static_cast<float>(luaL_checknumber(L, 2));
	}
	lua_pushnumber(L, c.cmyk.m);
	return 1;
}
static int colorYellow(lua_State *L) {
	Color &c = checkColor(L, 1);
	if (lua_type(L, 2) == LUA_TNUMBER) {
		c.cmyk.y = static_cast<float>(luaL_checknumber(L, 2));
	}
	lua_pushnumber(L, c.cmyk.y);
	return 1;
}
static int colorKeyBlack(lua_State *L) {
	Color &c = checkColor(L, 1);
	if (lua_type(L, 2) == LUA_TNUMBER) {
		c.cmyk.k = static_cast<float>(luaL_checknumber(L, 2));
	}
	lua_pushnumber(L, c.cmyk.k);
	return 1;
}
static int colorLab(lua_State *L) {
	Color &c = checkColor(L, 1);
	if (lua_type(L, 2) == LUA_TNUMBER && lua_type(L, 3) == LUA_TNUMBER && lua_type(L, 4) == LUA_TNUMBER) {
		c.lab.L = static_cast<float>(luaL_checknumber(L, 2));
		c.lab.a = static_cast<float>(luaL_checknumber(L, 3));
		c.lab.b = static_cast<float>(luaL_checknumber(L, 4));
	}
	lua_pushnumber(L, c.lab.L);
	lua_pushnumber(L, c.lab.a);
	lua_pushnumber(L, c.lab.b);
	return 3;
}
static int colorLabLightness(lua_State *L) {
	Color &c = checkColor(L, 1);
	if (lua_type(L, 2) == LUA_TNUMBER) {
		c.lab.L = static_cast<float>(luaL_checknumber(L, 2));
	}
	lua_pushnumber(L, c.lab.L);
	return 1;
}
static int colorLabA(lua_State *L) {
	Color &c = checkColor(L, 1);
	if (lua_type(L, 2) == LUA_TNUMBER) {
		c.lab.a = static_cast<float>(luaL_checknumber(L, 2));
	}
	lua_pushnumber(L, c.lab.a);
	return 1;
}
static int colorLabB(lua_State *L) {
	Color &c = checkColor(L, 1);
	if (lua_type(L, 2) == LUA_TNUMBER) {
		c.lab.b = static_cast<float>(luaL_checknumber(L, 2));
	}
	lua_pushnumber(L, c.lab.b);
	return 1;
}
static int colorOklab(lua_State *L) {
	Color &c = checkColor(L, 1);
	if (lua_type(L, 2) == LUA_TNUMBER && lua_type(L, 3) == LUA_TNUMBER && lua_type(L, 4) == LUA_TNUMBER) {
		c.oklab.L = static_cast<float>(luaL_checknumber(L, 2));
		c.oklab.a = static_cast<float>(luaL_checknumber(L, 3));
		c.oklab.b = static_cast<float>(luaL_checknumber(L, 4));
	}
	lua_pushnumber(L, c.oklab.L);
	lua_pushnumber(L, c.oklab.a);
	lua_pushnumber(L, c.oklab.b);
	return 3;
}
static int colorOklabLightness(lua_State *L) {
	Color &c = checkColor(L, 1);
	if (lua_type(L, 2) == LUA_TNUMBER) {
		c.oklab.L = static_cast<float>(luaL_checknumber(L, 2));
	}
	lua_pushnumber(L, c.lab.L);
	return 1;
}
static int colorOklabA(lua_State *L) {
	Color &c = checkColor(L, 1);
	if (lua_type(L, 2) == LUA_TNUMBER) {
		c.oklab.a = static_cast<float>(luaL_checknumber(L, 2));
	}
	lua_pushnumber(L, c.lab.a);
	return 1;
}
static int colorOklabB(lua_State *L) {
	Color &c = checkColor(L, 1);
	if (lua_type(L, 2) == LUA_TNUMBER) {
		c.oklab.b = static_cast<float>(luaL_checknumber(L, 2));
	}
	lua_pushnumber(L, c.lab.b);
	return 1;
}
static int colorRgbToHsl(lua_State *L) {
	pushColor(L, checkColor(L, 1).rgbToHsl());
	return 1;
}
static int colorRgbToHsv(lua_State *L) {
	pushColor(L, checkColor(L, 1).rgbToHsv());
	return 1;
}
static int colorHslToRgb(lua_State *L) {
	pushColor(L, checkColor(L, 1).hslToRgb());
	return 1;
}
static int colorHsvToRgb(lua_State *L) {
	pushColor(L, checkColor(L, 1).hsvToRgb());
	return 1;
}
static int colorRgbToCmyk(lua_State *L) {
	pushColor(L, checkColor(L, 1).rgbToCmyk());
	return 1;
}
static int colorCmykToRgb(lua_State *L) {
	pushColor(L, checkColor(L, 1).cmykToRgb());
	return 1;
}
static int colorRgbToLab(lua_State *L) {
	pushColor(L, checkColor(L, 1).rgbToLabD50());
	return 1;
}
static int colorRgbToLch(lua_State *L) {
	pushColor(L, checkColor(L, 1).rgbToLchD50());
	return 1;
}
static int colorRgbToOklab(lua_State *L) {
	pushColor(L, checkColor(L, 1).rgbToOklab());
	return 1;
}
static int colorRgbToOklch(lua_State *L) {
	pushColor(L, checkColor(L, 1).rgbToOklch());
	return 1;
}
static int colorLabToRgb(lua_State *L) {
	pushColor(L, checkColor(L, 1).labToRgbD50());
	return 1;
}
static int colorLchToRgb(lua_State *L) {
	pushColor(L, checkColor(L, 1).lchToRgbD50());
	return 1;
}
static int colorOklabToRgb(lua_State *L) {
	pushColor(L, checkColor(L, 1).oklabToRgb());
	return 1;
}
static int colorOklchToRgb(lua_State *L) {
	pushColor(L, checkColor(L, 1).oklchToRgb());
	return 1;
}
static int colorLch(lua_State *L) {
	Color &c = checkColor(L, 1);
	if (lua_type(L, 2) == LUA_TNUMBER && lua_type(L, 3) == LUA_TNUMBER && lua_type(L, 4) == LUA_TNUMBER) {
		c.lch.L = static_cast<float>(luaL_checknumber(L, 2));
		c.lch.C = static_cast<float>(luaL_checknumber(L, 3));
		c.lch.h = static_cast<float>(luaL_checknumber(L, 4));
	}
	lua_pushnumber(L, c.lch.L);
	lua_pushnumber(L, c.lch.C);
	lua_pushnumber(L, c.lch.h);
	return 3;
}
static int colorLchLightness(lua_State *L) {
	Color &c = checkColor(L, 1);
	if (lua_type(L, 2) == LUA_TNUMBER) {
		c.lch.L = static_cast<float>(luaL_checknumber(L, 2));
	}
	lua_pushnumber(L, c.lch.L);
	return 1;
}
static int colorLchChroma(lua_State *L) {
	Color &c = checkColor(L, 1);
	if (lua_type(L, 2) == LUA_TNUMBER) {
		c.lch.C = static_cast<float>(luaL_checknumber(L, 2));
	}
	lua_pushnumber(L, c.lch.C);
	return 1;
}
static int colorLchHue(lua_State *L) {
	Color &c = checkColor(L, 1);
	if (lua_type(L, 2) == LUA_TNUMBER) {
		c.lch.h = static_cast<float>(luaL_checknumber(L, 2));
	}
	lua_pushnumber(L, c.lch.h);
	return 1;
}
static int colorOklch(lua_State *L) {
	Color &c = checkColor(L, 1);
	if (lua_type(L, 2) == LUA_TNUMBER && lua_type(L, 3) == LUA_TNUMBER && lua_type(L, 4) == LUA_TNUMBER) {
		c.oklch.L = static_cast<float>(luaL_checknumber(L, 2));
		c.oklch.C = static_cast<float>(luaL_checknumber(L, 3));
		c.oklch.h = static_cast<float>(luaL_checknumber(L, 4));
	}
	lua_pushnumber(L, c.oklch.L);
	lua_pushnumber(L, c.oklch.C);
	lua_pushnumber(L, c.oklch.h);
	return 3;
}

static int colorOklchLightness(lua_State *L) {
	Color &c = checkColor(L, 1);
	if (lua_type(L, 2) == LUA_TNUMBER) {
		c.oklch.L = static_cast<float>(luaL_checknumber(L, 2));
	}
	lua_pushnumber(L, c.lch.L);
	return 1;
}
static int colorOklchChroma(lua_State *L) {
	Color &c = checkColor(L, 1);
	if (lua_type(L, 2) == LUA_TNUMBER) {
		c.oklch.C = static_cast<float>(luaL_checknumber(L, 2));
	}
	lua_pushnumber(L, c.lch.C);
	return 1;
}
static int colorOklchHue(lua_State *L) {
	Color &c = checkColor(L, 1);
	if (lua_type(L, 2) == LUA_TNUMBER) {
		c.oklch.h = static_cast<float>(luaL_checknumber(L, 2));
	}
	lua_pushnumber(L, c.lch.h);
	return 1;
}
static const struct luaL_Reg color_functions[] = {
	{ "new", newColor },
	{ nullptr, nullptr }
};
static const struct luaL_Reg color_members[] = {
	{ "__tostring", toString },
	{ "red", colorRed },
	{ "green", colorGreen },
	{ "blue", colorBlue },
	{ "alpha", colorAlpha },
	{ "rgb", colorRgb },
	{ "rgba", colorRgba },
	{ "hue", colorHue },
	{ "saturation", colorSaturation },
	{ "lightness", colorLightness },
	{ "value", colorLightness },
	{ "hsl", colorHsl },
	{ "hsla", colorHsla },
	{ "cyan", colorCyan },
	{ "magenta", colorMagenta },
	{ "yellow", colorYellow },
	{ "key_black", colorKeyBlack },
	{ "keyBlack", colorKeyBlack },
	{ "cmyk", colorCmyk },
	{ "lab", colorLab },
	{ "labLightness", colorLabLightness },
	{ "labA", colorLabA },
	{ "labB", colorLabB },
	{ "lch", colorLch },
	{ "lchLightness", colorLchLightness },
	{ "lchChroma", colorLchChroma },
	{ "lchHue", colorLchHue },
	{ "oklab", colorOklab },
	{ "oklabLightness", colorOklabLightness },
	{ "oklabA", colorOklabA },
	{ "oklabB", colorOklabB },
	{ "oklch", colorOklch },
	{ "oklchLightness", colorOklchLightness },
	{ "oklchChroma", colorOklchChroma },
	{ "oklchHue", colorOklchHue },
	{ "rgbToHsl", colorRgbToHsl },
	{ "rgbToHsv", colorRgbToHsv },
	{ "rgbToCmyk", colorRgbToCmyk },
	{ "rgbToLab", colorRgbToLab },
	{ "rgbToLch", colorRgbToLch },
	{ "rgbToOklab", colorRgbToOklab },
	{ "rgbToOklch", colorRgbToOklch },
	{ "hslToRgb", colorHslToRgb },
	{ "hsvToRgb", colorHsvToRgb },
	{ "cmykToRgb", colorCmykToRgb },
	{ "labToRgb", colorLabToRgb },
	{ "lchToRgb", colorLchToRgb },
	{ "oklabToRgb", colorOklabToRgb },
	{ "oklchToRgb", colorOklchToRgb },
	{ nullptr, nullptr }
};
int registerColor(lua_State *L) {
	luaL_newmetatable(L, "color");
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	luaL_setfuncs(L, color_members, 0);
	lua_pop(L, 1);
	luaL_newlib(L, color_functions);
	return 1;
}
}
