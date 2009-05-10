
#include "LuaExt.h"
#include <glib.h>


static int lua_newcolor (lua_State *L) {
	Color *c = (Color*)lua_newuserdata(L, sizeof(Color));
	luaL_getmetatable(L, "color");
	lua_setmetatable(L, -2);
	color_zero(c);
	return 1;
}

Color *lua_checkcolor (lua_State *L) {
	void *ud = luaL_checkudata(L, 1, "color");
	luaL_argcheck(L, ud != NULL, 1, "`color' expected");
	return (Color *)ud;
}

int lua_pushcolor (lua_State *L, Color* color) {
	Color *c = (Color*)lua_newuserdata(L, sizeof(Color));
	luaL_getmetatable(L, "color");
	lua_setmetatable(L, -2);
	color_copy(color, c);
	return 1;
}

static int lua_color2string (lua_State *L) {
	Color *c = lua_checkcolor(L);
	lua_pushfstring(L, "color(%f, %f, %f)", c->rgb.red, c->rgb.green, c->rgb.blue);
	return 1;
}

static int lua_color_red (lua_State *L) {
	Color *c = lua_checkcolor(L);
	if (lua_type(L, 1)==LUA_TNUMBER){
		c->rgb.red=luaL_checknumber(L, 1);
	}
	lua_pushnumber(L, c->rgb.red);
	return 1;
}

static int lua_color_green (lua_State *L) {
	Color *c = lua_checkcolor(L);
	if (lua_type(L, 1)==LUA_TNUMBER){
		c->rgb.green=luaL_checknumber(L, 1);
	}
	lua_pushnumber(L, c->rgb.green);
	return 1;
}

static int lua_color_blue (lua_State *L) {
	Color *c = lua_checkcolor(L);
	if (lua_type(L, 1)==LUA_TNUMBER){
		c->rgb.blue=luaL_checknumber(L, 1);
	}
	lua_pushnumber(L, c->rgb.blue);
	return 1;
}

static int lua_color_hue (lua_State *L) {
	Color *c = lua_checkcolor(L);
	if (lua_type(L, 1)==LUA_TNUMBER){
		c->hsl.hue=luaL_checknumber(L, 1);
	}
	lua_pushnumber(L, c->hsl.hue);
	return 1;
}

static int lua_color_saturation (lua_State *L) {
	Color *c = lua_checkcolor(L);
	if (lua_type(L, 1)==LUA_TNUMBER){
		c->hsl.saturation=luaL_checknumber(L, 1);
	}
	lua_pushnumber(L, c->hsl.saturation);
	return 1;
}

static int lua_color_lightness (lua_State *L) {
	Color *c = lua_checkcolor(L);
	if (lua_type(L, 1)==LUA_TNUMBER){
		c->hsl.lightness=luaL_checknumber(L, 1);
	}
	lua_pushnumber(L, c->hsl.lightness);
	return 1;
}


static int lua_color_rgb_to_hsl (lua_State *L) {
	Color *c = lua_checkcolor(L);
	Color c2;
	color_rgb_to_hsl(c, &c2);
	lua_pushcolor(L, &c2);
	return 1;
}

static int lua_color_hsl_to_rgb (lua_State *L) {
	Color *c = lua_checkcolor(L);
	Color c2;
	color_hsl_to_rgb(c, &c2);
	lua_pushcolor(L, &c2);
	return 1;
}

static const struct luaL_reg lua_colorlib_f [] = {
	{"new", lua_newcolor},
	{NULL, NULL}
};

static const struct luaL_reg lua_colorlib_m [] = {
	{"__tostring", lua_color2string},
	{"red",			lua_color_red},
	{"green",		lua_color_green},
	{"blue",		lua_color_blue},
	
	{"hue",			lua_color_hue},
	{"saturation",	lua_color_saturation},
	{"lightness",	lua_color_lightness},
	
	{"rgb_to_hsl",	lua_color_rgb_to_hsl},
	{"hsl_to_rgb",	lua_color_hsl_to_rgb},
	
	{NULL, NULL}
};


static int luaopen_color (lua_State *L) {
	luaL_newmetatable(L, "color");

	lua_pushstring(L, "__index");
	lua_pushvalue(L, -2);  /* pushes the metatable */
	lua_settable(L, -3);  /* metatable.__index = metatable */

	luaL_openlib(L, NULL, lua_colorlib_m, 0);
	luaL_openlib(L, "color", lua_colorlib_f, 0);
		
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

static const struct luaL_reg lua_colorobjectlib_f [] = {
	{"new", lua_newcolorobject},
	{NULL, NULL}
};

static const struct luaL_reg lua_colorobjectlib_m [] = {
	{"get_color", lua_colorobject_get_color},
	{NULL, NULL}
};


int luaopen_colorobject (lua_State *L) {
	luaL_newmetatable(L, "colorobject");

	lua_pushstring(L, "__index");
	lua_pushvalue(L, -2);  /* pushes the metatable */
	lua_settable(L, -3);  /* metatable.__index = metatable */

	luaL_openlib(L, NULL, lua_colorobjectlib_m, 0);
	luaL_openlib(L, "colorobject", lua_colorobjectlib_f, 0);
		
	return 1;
}


int LuaExt_Colors(struct LuaSystem* lua, void* userdata){
    lua_pushcfunction(lua->l, luaopen_color);
    lua_pushstring(lua->l, 0);
    lua_call(lua->l, 1, 0);
    
	lua_pushcfunction(lua->l, luaopen_colorobject);
    lua_pushstring(lua->l, 0);
    lua_call(lua->l, 1, 0); 
    return 0;
}

