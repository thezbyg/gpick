
#ifndef LUAEXT_H_
#define LUAEXT_H_

#include "LuaSystem.h"
#include "Color.h"
#include "ColorObject.h"

int LuaExt_Colors(struct LuaSystem* lua, void* userdata);

int lua_pushcolorobject (lua_State *L, struct ColorObject* color_object);
struct ColorObject** lua_checkcolorobject (lua_State *L, int index);

#endif /* LUAEXT_H_ */
