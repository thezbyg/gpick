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

#include "Converter.h"
#include "GlobalState.h"
#include "ColorObject.h"
#include "lua/Color.h"
#include "lua/ColorObject.h"
#include "lua/Script.h"
#include <string.h>
#include <stdlib.h>
#include <glib.h>
#include <fstream>
#include <string>
#include <sstream>
#include <map>
#include <list>
#include <vector>
#include <iostream>
#include <functional>
using namespace std;
extern "C"{
#include <lualib.h>
#include <lauxlib.h>
}
Converter::Converter(const char *name, const char *label, lua::Ref &&serialize, lua::Ref &&deserialize):
	m_name(name),
	m_label(label),
	m_serialize(move(serialize)),
	m_deserialize(move(deserialize)),
	m_copy(false),
	m_paste(false)
{
}
std::string Converter::serialize(const ColorObject &colorObject, const ConverterSerializePosition &position) {
	if (!m_serialize.valid())
		return "";
	lua_State *L = m_serialize.script();
	int stack_top = lua_gettop(L);
	m_serialize.get();
	lua::pushColorObject(L, colorObject.copy());
	lua_newtable(L);
	lua_pushboolean(L, position.first());
	lua_setfield(L, -2, "first");
	lua_pushboolean(L, position.last());
	lua_setfield(L, -2, "last");
	lua_pushinteger(L, position.index());
	lua_setfield(L, -2, "index");
	lua_pushinteger(L, position.count());
	lua_setfield(L, -2, "count");
	int status = lua_pcall(L, 2, 1, 0);
	if (status == 0){
		if (lua_type(L, -1) == LUA_TSTRING){
			string result = luaL_checkstring(L, -1);
			lua_settop(L, stack_top);
			return result;
		}else{
			cerr << "serialize: returned not a string value \"" << m_name << "\"" << endl;
		}
	}else{
		cerr << "serialize: " << lua_tostring(L, -1) << endl;
	}
	lua_settop(L, stack_top);
	return "";
}
std::string Converter::serialize(const ColorObject *color_object, const ConverterSerializePosition &position)
{
	return serialize(*color_object, position);
}
bool Converter::deserialize(const char *value, ColorObject *color_object, float &quality)
{
	if (!m_deserialize.valid())
		return "";
	lua_State *L = m_deserialize.script();
	int stack_top = lua_gettop(L);
	m_deserialize.get();
	lua_pushstring(L, value);
	lua::pushColorObject(L, color_object);
	int status = lua_pcall(L, 2, 1, 0);
	if (status == 0){
		if (lua_type(L, -1) == LUA_TNUMBER){
			quality = static_cast<float>(luaL_checknumber(L, -1));
			lua_settop(L, stack_top);
			return true;
		}else{
			cerr << "deserialize: returned not a number value \"" << m_name <<"\"" << endl;
		}
	}else{
		cerr << "deserialize: " << lua_tostring(L, -1) << endl;
	}
	lua_settop(L, stack_top);
	return false;
}
std::string Converter::serialize(const ColorObject *color_object)
{
	ConverterSerializePosition position;
	return serialize(color_object, position);
}
std::string Converter::serialize(const ColorObject &colorObject) {
	ConverterSerializePosition position;
	return serialize(colorObject, position);
}
std::string Converter::serialize(const Color &color)
{
	ColorObject color_object("", color);
	ConverterSerializePosition position;
	return serialize(&color_object, position);
}
const std::string &Converter::name() const
{
	return m_name;
}
const std::string &Converter::label() const
{
	return m_label;
}
bool Converter::hasSerialize() const
{
	return m_serialize.valid();
}
bool Converter::hasDeserialize() const
{
	return m_deserialize.valid();
}
void Converter::copy(bool value)
{
	m_copy = value;
}
void Converter::paste(bool value)
{
	m_paste = value;
}
bool Converter::copy() const
{
	return m_copy;
}
bool Converter::paste() const
{
	return m_paste;
}

ConverterSerializePosition::ConverterSerializePosition():
	m_first(true),
	m_last(true),
	m_index(0),
	m_count(1)
{
}
ConverterSerializePosition::ConverterSerializePosition(size_t count):
	m_first(true),
	m_last(count <= 1),
	m_index(0),
	m_count(count)
{
}
bool ConverterSerializePosition::first() const
{
	return m_first;
}
bool ConverterSerializePosition::last() const
{
	return m_last;
}
size_t ConverterSerializePosition::index() const
{
	return m_index;
}
size_t ConverterSerializePosition::count() const
{
	return m_count;
}
void ConverterSerializePosition::incrementIndex()
{
	m_index++;
}
void ConverterSerializePosition::first(bool value)
{
	m_first = value;
}
void ConverterSerializePosition::last(bool value)
{
	m_last = value;
}
