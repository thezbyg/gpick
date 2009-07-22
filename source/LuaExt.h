
#ifndef LUAEXT_H_
#define LUAEXT_H_

extern "C"{
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <luaconf.h>
}

#include "Color.h"
#include "ColorObject.h"

int lua_ext_colors_openlib(lua_State *lua);
int lua_pushcolorobject (lua_State *L, struct ColorObject* color_object);
struct ColorObject** lua_checkcolorobject (lua_State *L, int index);

#endif /* LUAEXT_H_ */
