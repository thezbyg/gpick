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

#include "LuaSystem.h"

#include <string.h>
#include <stdlib.h>

#include <math.h>
#include <sstream>
#include <iostream>
using namespace std;

LUALIB_API int luaopen_luasys (lua_State *L);

#define toproto(L,i) (clvalue(L->top+(i))->l.p)

struct lua_compile_buffer{
	char* data;
	unsigned int used,size;
};

static int compile_writer(lua_State* L, const void* p, size_t size, void* u) {
	struct lua_compile_buffer* cb = (struct lua_compile_buffer*) u;
	if (size <= 0)
		return 0;
	if (cb->size - cb->used < size) {
		char* newdata;
		cb->size = cb->size + ((size / 4096) + 1) * 4096;
		newdata = new char[cb->size];
		if (cb->used)
			memcpy(newdata, cb->data, cb->used);
		memcpy(&newdata[cb->used], p, size);
		cb->used += size;
		if (cb->data)
			delete[] cb->data;
		cb->data = newdata;
		return 0;
	} else {
		memcpy(&cb->data[cb->used], p, size);
		cb->used += size;
		return 0;
	}
	return 1;
}

#ifdef WIN32
#define strdup _strdup
#endif

int compile(char* name, char* code, unsigned int size, char** out, unsigned int* osize, char** message) {
	lua_State* L;
	L = lua_open();
	if (luaL_loadbuffer(L, code, size, name)) {
		*message = strdup(lua_tostring(L,-1));
		lua_close(L);
		return 0;
	}

	struct lua_compile_buffer buf;
	buf.size = 0;
	buf.data = 0;
	buf.used = 0;

	if (lua_dump(L, compile_writer, &buf) == 0) {
		if (buf.used) {
			(*out) = new char[buf.used];
			memcpy((*out), buf.data, buf.used);
			*osize = buf.used;
			delete[] buf.data;
		}
	}
	lua_close(L);
	return 1;
}

static const luaL_reg lualibs[] = {
  {"", luaopen_base},
  {LUA_LOADLIBNAME, luaopen_package},
  {LUA_TABLIBNAME, luaopen_table},
  {LUA_IOLIBNAME, luaopen_io},
  {LUA_OSLIBNAME, luaopen_os},
  {LUA_STRLIBNAME, luaopen_string},
  {LUA_MATHLIBNAME, luaopen_math},
  {LUA_DBLIBNAME, luaopen_debug},
  {0, luaopen_luasys},
  /* add your libraries here */
  {NULL, NULL}
};


static int luasys_report(lua_State *L, int status, char** message) {
	if (status && !lua_isnil(L, -1)) {
		const char *msg = lua_tostring(L, -1);
		if (msg == NULL)
			msg = "(error object is not a string)";
		//l_message(msg);
		*message = strdup(lua_tostring(L,-1));
		lua_pop(L, 1);
	}
	return status;
}


#define LEVELS1	12	/* size of the first part of the stack */
#define LEVELS2	10	/* size of the second part of the stack */

static int luasys_traceback(lua_State *l) {
	lua_getfield(l, LUA_GLOBALSINDEX, "debug");
	if (!lua_istable(l, -1)) {
		lua_pop(l, 1);
		return 1;
	}
	lua_getfield(l, -1, "traceback");
	if (!lua_isfunction(l, -1)) {
		lua_pop(l, 2);
		return 1;
	}
	lua_pushvalue(l, 1); /* pass error message */
	lua_pushinteger(l, 2); /* skip this function and traceback */
	lua_call(l, 2, 1); /* call debug.traceback */
	return 1;
}



static int luasys_docall(lua_State *L, int narg, int clear) {
	int status;
	int base = lua_gettop(L) - narg; /* function index */
	lua_pushcfunction(L, luasys_traceback); /* push traceback function */
	lua_insert(L, base); /* put it under chunk and args */
	status = lua_pcall(L, narg, (clear ? 0 : LUA_MULTRET), base);
	lua_remove(L, base); /* remove traceback function */
	if (status != 0)
		lua_gc(L, LUA_GCCOLLECT, 0);
	return status;
}

static int luasys_dostring (lua_State *L, const char *s, unsigned int size, const char *name,char** message) {
  int status = luaL_loadbuffer(L, s, size, name) || luasys_docall(L, 0, 1);
  return luasys_report(L, status,message);
}



struct LuaSystem* luasysCreate(){
	struct LuaSystem* lua;
	lua=new struct LuaSystem;
	lua->l=0;
	return lua;
}

int luasysClear(struct LuaSystem* lua){
	if (lua->l) lua_close(lua->l);
	lua->l=0;
	return 0;
}

int luasysDestroy(struct LuaSystem* lua){
	if (lua->l) lua_close(lua->l);
	delete lua;
	return 0;
}


typedef struct luasysSmain {
  const char* buf;
  long size;
  const char* file;
  struct LuaSystem* lua;
  int status;

  luasys_callback callback;
  void* userdata;
}luasysSmain;

static void luasys_openstdlibs (lua_State *l) {
  const luaL_reg *lib = lualibs;
  for (; lib->func; lib++) {
    lua_pushcfunction(l, lib->func);
    lua_pushstring(l, lib->name);
    lua_call(l, 1, 0);
  }
}

static int luasys_compile (lua_State *l) {
	luasysSmain *s = (luasysSmain *)lua_touserdata(l, 1);
	int status=0;

	lua_gc(l, LUA_GCSTOP, 0);  /* stop collector during initialization */
	luasys_openstdlibs(l);  /* open libraries */

	if (s->callback) s->callback(s->lua,s->userdata);

	lua_gc(l, LUA_GCRESTART, 0);

	char* message=0;

	luasys_dostring(l,s->buf,s->size,s->file,&message);

	if (message){
		cout<<"luasys_compile: "<<message<<endl;
		free(message);
		s->status=-1;
	}

	return status;
}


int luasysFree(void* ptr){
	delete [] (unsigned char*)ptr;
	return 0;
}

int luasysCompile(struct LuaSystem* lua, const char* name, const char* code, unsigned long size,luasys_callback callback,void* userdata){

	lua->l=lua_open();
	if (lua->l){

		int status=0;
		luasysSmain s;
		s.buf=code;
		s.size=size;
		s.file=name;
		s.lua=lua;
		s.callback=callback;
		s.userdata=userdata;
		s.status=0;

		status = lua_cpcall(lua->l, &luasys_compile, &s);

		char* message=0;

		if (status){
			luasys_report(lua->l,status,&message);

			if (message){
				cout<<"luasysCompile: "<<message<<endl;
				free(message);
			}
			status=-1;
		}

		status=s.status;


		return status;
	}

	return -1;
}

int luasysCall(struct LuaSystem* lua, const char* function){
	int status=0;

	char* message=0;

	luasys_dostring(lua->l,function,strlen(function),"dynamic_script",&message);

	if (message){
		cout<<"luasysCall: "<<message<<endl;
		free(message);
	}
	return status;
}

/*
#define LUA_TNIL		0
#define LUA_TBOOLEAN		1
#define LUA_TLIGHTUSERDATA	2
#define LUA_TNUMBER		3
#define LUA_TSTRING		4
#define LUA_TTABLE		5
#define LUA_TFUNCTION		6
#define LUA_TUSERDATA		7
#define LUA_TTHREAD		8


LUA_API void  (lua_pushnil) (lua_State *L);
LUA_API void  (lua_pushnumber) (lua_State *L, lua_Number n);
LUA_API void  (lua_pushinteger) (lua_State *L, lua_Integer n);
LUA_API void  (lua_pushlstring) (lua_State *L, const char *s, size_t l);
LUA_API void  (lua_pushstring) (lua_State *L, const char *s);
LUA_API const char *(lua_pushvfstring) (lua_State *L, const char *fmt,
                                                      va_list argp);
LUA_API const char *(lua_pushfstring) (lua_State *L, const char *fmt, ...);
LUA_API void  (lua_pushcclosure) (lua_State *L, lua_CFunction fn, int n);
LUA_API void  (lua_pushboolean) (lua_State *L, int b);
LUA_API void  (lua_pushlightuserdata) (lua_State *L, void *p);
LUA_API int   (lua_pushthread) (lua_State *L);

  */
void *luaL_checkuser(lua_State *L, int numArg);

int luasysCallParams(struct LuaSystem* lua, const char* function,luasys_callparam* param,unsigned long params,luasys_callparam* param_ret,unsigned long params_ret,int silent_failure){


	int status=0;

	lua_getglobal(lua->l,function);

	for (unsigned long i=0;i<params;i++){
		switch (param[i].type){
		case LUA_TLIGHTUSERDATA:
			lua_pushlightuserdata(lua->l,param[i].param);
			break;
		case LUA_TNIL:
			lua_pushnil(lua->l);
			break;
		case LUA_TSTRING:
			lua_pushstring(lua->l,(char*)param[i].param);
			break;
		case LUA_TNUMBER:
			lua_pushnumber(lua->l,*(double*)param[i].param);
			break;
		default:
			lua_pushnil(lua->l);
		}
	}

	status=lua_pcall(lua->l, params, params_ret, 0);

	char* message=0;

	if (status){
		luasys_report(lua->l,status,&message);

		if (message){
			if (!silent_failure) cout<<"luasysCallParams: "<<message<<endl;
			free(message);
		}
		status=-1;
	}else{
		size_t st;
		for (unsigned long i=0;i<params_ret;i++){
			switch (param_ret[i].type){
			case LUA_TLIGHTUSERDATA:
				param_ret[i].param=luaL_checkuser(lua->l,-1);
				lua_pop(lua->l, 1);
				break;
			case LUA_TSTRING:
				param_ret[i].param=(void*)luaL_checklstring(lua->l,-1,&st);
				lua_pop(lua->l, 1);
				break;
			case LUA_TNUMBER:
				*(double*)param_ret[i].param=luaL_checknumber(lua->l,-1);
				lua_pop(lua->l, 1);
				break;
			case LUA_TBOOLEAN:
				*(long*)param_ret[i].param=luaL_checkinteger(lua->l,-1);
				lua_pop(lua->l, 1);
				break;
			default:
				break;
			}
		}
	}
	return status;
}

void *luaL_checkuser(lua_State *L, int numArg){
	return lua_touserdata(L, numArg);
}

void *luaL_optuser(lua_State *L, int numArg,void* def){
	if (lua_isuserdata(L,numArg)){
		return lua_touserdata(L, numArg);
	}else return def;
}

int argcount(lua_State *L) {
	return lua_gettop(L);
}


static const luaL_reg luasyslib[] = {

	{NULL, NULL}
};


LUALIB_API int luaopen_luasys (lua_State *L) {
	lua_pushliteral(L, "_G");
	lua_pushvalue(L, LUA_GLOBALSINDEX);
	luaL_register(L, 0, luasyslib);
	lua_rawset(L, -1);  /* set global _G */
	return 1;
}











