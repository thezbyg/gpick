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

#ifndef LUASYS_H_
#define LUASYS_H_

typedef struct luasys luasys;
typedef struct luasys_callparam luasys_callparam;

extern "C"{
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <luaconf.h>
}

struct LuaSystem{
	lua_State* l;
};

struct LuaSystem* luasysCreate();
int luasysDestroy(struct LuaSystem* lua);

int luasysClear(struct LuaSystem* lua);

typedef int (*luasys_callback)(struct LuaSystem* lua, void* userdata);

int luasysCompile(struct LuaSystem* lua, const char* name, const char* code, unsigned long size,luasys_callback callback=0,void* userdata=0);
int luasysCall(struct LuaSystem* lua, const char* function);

int luasysDump(struct LuaSystem* lua, unsigned char** data, unsigned long* size);
int luasysFree(void* ptr);

int luasys_report(lua_State *L, int status, char** message);

typedef struct luasys_callparam{
	void* param;
	unsigned long type;
}luasys_callparam;

int luasysCallParams(struct LuaSystem* lua, const char* function,luasys_callparam* param,unsigned long params,luasys_callparam* param_ret,unsigned long params_ret,int silent_failure=0);

void *luaL_checkuser(lua_State *L, int numArg);

#endif /*LUASYS_H_*/
