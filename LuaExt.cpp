
#include "LuaExt.h"
#include <glib.h>

static const luaL_reg luaext_lib[] = {
	{0, 0}
};

LUALIB_API int luaopen_luaext (lua_State *L) {
	luaL_register(L, "color" , luaext_lib);
	return 1;
}

int LuaExt_Colors(struct LuaSystem* lua, void* userdata){
    lua_pushcfunction(lua->l, luaopen_luaext);
    lua_pushstring(lua->l, 0);
    lua_call(lua->l, 1, 0);
    return 0;
}

