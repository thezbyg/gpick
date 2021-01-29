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

#include "Script.h"
#include <sstream>
extern "C"{
#include <lualib.h>
#include <lauxlib.h>
}
#include <iostream>
using namespace std;
namespace lua
{
Script::Script()
{
	m_state = luaL_newstate();
	m_state_owned = true;
	luaL_openlibs(m_state);
}
Script::Script(lua_State *state)
{
	m_state = state;
	m_state_owned = false;
}
Script::~Script()
{
	if (m_state_owned)
		lua_close(m_state);
	m_state = nullptr;
}
Script::operator lua_State*()
{
	return m_state;
}
static void trimSemicolon(std::string &value) {
	if (value.length() > 0 && value.back() == ';')
		value.resize(value.length() - 1);
}
static void removeCurrentDirectory(std::string &value) {
	auto i = value.rfind("./?");
	if (i == std::string::npos)
		return;
	value.resize(i);
	trimSemicolon(value);
}
void Script::setPaths(const std::vector<std::string> &include_paths)
{
	string paths, cPaths;
	for (vector<string>::const_iterator i = include_paths.begin(); i != include_paths.end(); i++){
		if (i->empty())
			continue;
		paths += *i;
		paths += "/?.lua;";
		cPaths += *i;
		cPaths += "/?.so;";
	}
	trimSemicolon(paths);
	trimSemicolon(cPaths);
	lua_getglobal(m_state, "package");
	lua_pushstring(m_state, "path");
	lua_pushstring(m_state, paths.c_str());
	lua_settable(m_state, -3);
	lua_pushstring(m_state, "cpath");
	lua_gettable(m_state, -2);
	string currentCPaths = lua_tostring(m_state, -1);
	lua_pop(m_state, 1);
	currentCPaths = cPaths + ";" + currentCPaths;
	removeCurrentDirectory(currentCPaths);
	lua_pushstring(m_state, "cpath");
	lua_pushstring(m_state, currentCPaths.c_str());
	lua_settable(m_state, -3);
	lua_pop(m_state, 1);
}
bool Script::load(const char *script_name)
{
	int status;
	lua_getglobal(m_state, "require");
	lua_pushstring(m_state, script_name);
	status = lua_pcall(m_state, 1, 1, 0);
	if (status) {
		stringstream ss;
		ss << lua_tostring(m_state, -1);
		m_last_error = ss.str();
		return false;
	}
	return true;
}
bool Script::loadCode(const char *script_code)
{
	int status;
	status = luaL_loadstring(m_state, script_code);
	if (status){
		m_last_error = lua_tostring(m_state, -1);
		return false;
	}
	return true;
}
bool Script::run(int arguments_on_stack, int results)
{
	lua_State *L = m_state;
	int status;
	if (lua_type(L, -1) != LUA_TNIL){
		if ((status = lua_pcall(L, arguments_on_stack, results, 0))){
			stringstream ss;
			ss << "call failed: " << lua_tostring(L, -1);
			m_last_error = ss.str();
			return false;
		}else{
			return true;
		}
	}else{
		m_last_error = "requested function was not found";
		return false;
	}
}
static int registerLuaPackage(lua_State *L)
{
	lua_getglobal(L, "__script");
	auto &script = *reinterpret_cast<Script*>(lua_touserdata(L, -1));
	lua_getglobal(L, "__extension");
	auto &extension = *reinterpret_cast<function<int(Script &)>*>(lua_touserdata(L, -1));
	lua_pop(L, 2);
	return extension(script);
}
bool Script::registerExtension(const char *name, std::function<int(Script &)> extension)
{
	lua_State *L = m_state;
	string full_name;
	if (name == nullptr)
		full_name = "gpick";
	else
		full_name = string("gpick/") + name;
	lua_pushlightuserdata(L, this);
	lua_setglobal(L, "__script");
	lua_pushlightuserdata(L, &extension);
	lua_setglobal(L, "__extension");
	luaL_requiref(m_state, full_name.c_str(), registerLuaPackage, 0);
	lua_pushnil(L);
	lua_setglobal(L, "__script");
	lua_pushnil(L);
	lua_setglobal(L, "__extension");
	lua_pop(L, 1);
	return true;
}
std::string Script::getString(int index)
{
	return lua_tostring(m_state, index);
}
void Script::createType(const char *name, const luaL_Reg *members)
{
	lua_State *L = m_state;
	luaL_newmetatable(L, name);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	luaL_setfuncs(L, members, 0);
	lua_pop(L, 1);
}
const std::string &Script::getLastError()
{
	return m_last_error;
}
}
