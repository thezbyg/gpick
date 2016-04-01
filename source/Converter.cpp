/*
 * Copyright (c) 2009-2016, Albertas Vyšniauskas
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
#include "DynvHelpers.h"
#include "GlobalState.h"
#include "ColorObject.h"
#include "LuaExt.h"
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
using namespace std;
extern "C"{
#include <lualib.h>
#include <lauxlib.h>
}

class ConverterKeyCompare{
public:
	bool operator() (const char* const& x, const char* const& y) const;
};
bool ConverterKeyCompare::operator() (const char* const& x, const char* const& y) const
{
	return strcmp(x,y)<0;
}
class Converters{
public:
	typedef std::map<const char*, Converter*, ConverterKeyCompare> ConverterMap;
	ConverterMap converters;
	list<Converter*> all_converters;
	vector<Converter*> copy_converters;
	vector<Converter*> paste_converters;
	Converter* display_converter;
	Converter* color_list_converter;
	lua_State *L;
	struct dynvSystem* params;
	~Converters();
};
Converters::~Converters()
{
	Converters::ConverterMap::iterator i;
	for (i=converters.begin(); i != converters.end(); ++i){
		g_free(((*i).second)->human_readable);
		g_free(((*i).second)->function_name);
		delete ((*i).second);
	}
	converters.clear();
}
int converters_color_deserialize(Converters* converters, const char* function, const char* text, ColorObject* color_object, float* conversion_quality)
{
	lua_State* L = converters->L;
	int status;
	int stack_top = lua_gettop(L);
	lua_getglobal(L, "gpick");
	int gpick_namespace = lua_gettop(L);
	if (lua_type(L, -1) != LUA_TNIL){
		lua_pushstring(L, "color_deserialize");
		lua_gettable(L, gpick_namespace);
		if (lua_type(L, -1) != LUA_TNIL){
			lua_pushstring(L, function);
			lua_pushstring(L, text);
			lua_pushcolorobject (L, color_object);
			lua_pushdynvsystem(L, converters->params);
			status=lua_pcall(L, 4, 1, 0);
			dynv_system_release(converters->params);
			if (status == 0){
				if (lua_type(L, -1) == LUA_TNUMBER){
					double result = luaL_checknumber(L, -1);
					*conversion_quality = result;
					lua_settop(L, stack_top);
					return 0;
				}else{
					cerr<<"gpick.color_deserialize: returned not a number value \""<<function<<"\""<<endl;
				}
			}else{
				cerr<<"gpick.color_deserialize: "<<lua_tostring (L, -1)<<endl;
			}
		}else{
			cerr<<"gpick.color_deserialize: no such function \""<<function<<"\""<<endl;
		}
	}
	lua_settop(L, stack_top);
	return -1;
}
int converters_color_deserialize(Converter *converter, const char* text, ColorObject *color_object, float* conversion_quality)
{
	lua_State* L = converter->converters->L;
	int status;
	int stack_top = lua_gettop(L);
	lua_getglobal(L, "gpick");
	int gpick_namespace = lua_gettop(L);
	if (lua_type(L, -1) != LUA_TNIL){
		lua_pushstring(L, "color_deserialize");
		lua_gettable(L, gpick_namespace);
		if (lua_type(L, -1) != LUA_TNIL){
			lua_pushstring(L, converter->function_name);
			lua_pushstring(L, text);
			lua_pushcolorobject (L, color_object);
			lua_pushdynvsystem(L, converter->converters->params);
			status = lua_pcall(L, 4, 1, 0);
			dynv_system_release(converter->converters->params);
			if (status == 0){
				if (lua_type(L, -1) == LUA_TNUMBER){
					double result = luaL_checknumber(L, -1);
					*conversion_quality = result;
					lua_settop(L, stack_top);
					return 0;
				}else{
					cerr<<"gpick.color_deserialize: returned not a number value \""<<converter->function_name<<"\""<<endl;
				}
			}else{
				cerr<<"gpick.color_deserialize: "<<lua_tostring (L, -1)<<endl;
			}
		}else{
			cerr<<"gpick.color_deserialize: no such function \""<<converter->function_name<<"\""<<endl;
		}
	}
	lua_settop(L, stack_top);
	return -1;
}
int converters_color_serialize(Converters* converters, const char* function, const ColorObject* color_object, const ConverterSerializePosition &position, string& result)
{
	lua_State* L = converters->L;
	int status;
	int stack_top = lua_gettop(L);
	lua_getglobal(L, "gpick");
	int gpick_namespace = lua_gettop(L);
	if (lua_type(L, -1) != LUA_TNIL){
		lua_pushstring(L, "color_serialize");
		lua_gettable(L, gpick_namespace);
		if (lua_type(L, -1) != LUA_TNIL){
			lua_pushstring(L, function);
			lua_pushcolorobject(L, const_cast<ColorObject*>(color_object));
			lua_pushdynvsystem(L, converters->params);
			lua_newtable(L);
			lua_pushboolean(L, position.first);
			lua_setfield(L, -2, "first");
			lua_pushboolean(L, position.last);
			lua_setfield(L, -2, "last");
			lua_pushinteger(L, position.index);
			lua_setfield(L, -2, "index");
			lua_pushinteger(L, position.count);
			lua_setfield(L, -2, "count");
			status = lua_pcall(L, 4, 1, 0);
			dynv_system_release(converters->params);
			if (status == 0){
				if (lua_type(L, -1) == LUA_TSTRING){
					result = luaL_checkstring(L, -1);
					lua_settop(L, stack_top);
					return 0;
				}else{
					cerr << "gpick.color_serialize: returned not a string value \"" << function << "\"" << endl;
				}
			}else{
				cerr << "gpick.color_serialize: " << lua_tostring(L, -1) << endl;
			}
		}else{
			cerr << "gpick.color_serialize: no such function \"" << function << "\"" <<endl;
		}
	}
	lua_settop(L, stack_top);
	return -1;
}
int converters_color_serialize(Converter* converter, const ColorObject* color_object, const ConverterSerializePosition &position, std::string& result)
{
	lua_State* L = converter->converters->L;
	int status;
	int stack_top = lua_gettop(L);
	lua_getglobal(L, "gpick");
	int gpick_namespace = lua_gettop(L);
	if (lua_type(L, -1) != LUA_TNIL){
		lua_pushstring(L, "color_serialize");
		lua_gettable(L, gpick_namespace);
		if (lua_type(L, -1) != LUA_TNIL){
			lua_pushstring(L, converter->function_name);
			lua_pushcolorobject(L, const_cast<ColorObject*>(color_object));
			lua_pushdynvsystem(L, converter->converters->params);
			lua_newtable(L);
			lua_pushboolean(L, position.first);
			lua_setfield(L, -2, "first");
			lua_pushboolean(L, position.last);
			lua_setfield(L, -2, "last");
			lua_pushinteger(L, position.index);
			lua_setfield(L, -2, "index");
			lua_pushinteger(L, position.count);
			lua_setfield(L, -2, "count");
			status = lua_pcall(L, 4, 1, 0);
			dynv_system_release(converter->converters->params);
			if (status == 0){
				if (lua_type(L, -1) == LUA_TSTRING){
					result = luaL_checkstring(L, -1);
					lua_settop(L, stack_top);
					return 0;
				}else{
					cerr << "gpick.color_serialize: returned not a string value \"" << converter->function_name << "\"" << endl;
				}
			}else{
				cerr << "gpick.color_serialize: " << lua_tostring(L, -1) << endl;
			}
		}else{
			cerr << "gpick.color_serialize: no such function \"" << converter->function_name << "\"" <<endl;
		}
	}
	lua_settop(L, stack_top);
	return -1;
}
Converters* converters_init(lua_State *lua, dynvSystem *settings)
{
	if (lua == nullptr) return nullptr;
	lua_State* L = lua;
	Converters *converters = new Converters;
	converters->L = L;
	converters->display_converter = 0;
	converters->params = dynv_system_ref(settings);
	int stack_top = lua_gettop(L);
	lua_getglobal(L, "gpick");
	int gpick_namespace = lua_gettop(L);
	if (lua_type(L, -1) != LUA_TNIL){
		lua_pushstring(L, "converters");
		lua_gettable(L, gpick_namespace);
		int converters_table = lua_gettop(L);
		lua_pushnil(L);
		while (lua_next(L, converters_table) != 0){
			if (lua_type(L, -2) == LUA_TSTRING){
				Converter *converter = new Converter;
				converter->converters = converters;
				converter->function_name = g_strdup(lua_tostring(L, -2));
				converters->converters[converter->function_name] = converter;
				converters->all_converters.push_back(converter);
				lua_pushstring(L, "human_readable");
				lua_gettable(L, -2);
				converter->human_readable = g_strdup(lua_tostring(L, -1));
				lua_pop(L, 1);
				lua_pushstring(L, "serialize");
				lua_gettable(L, -2);
				converter->serialize_available = !lua_isnil(L, -1);
				converter->copy = false;
				lua_pop(L, 1);
				lua_pushstring(L, "deserialize");
				lua_gettable(L, -2);
				converter->deserialize_available = !lua_isnil(L, -1);
				converter->paste = false;
				lua_pop(L, 1);
			}
			lua_pop(L, 1); //pop value from stack, but leave key
		}
	}
	lua_settop(L, stack_top);
	return converters;
}
int converters_term(Converters *converters)
{
	dynv_system_release(converters->params);
	delete converters;
	return 0;
}
Converter* converters_get(Converters *converters, const char* name)
{
	Converters::ConverterMap::iterator i;
	i=converters->converters.find(name);
	if (i != converters->converters.end()){
		return (*i).second;
	}else{
		return 0;
	}
}
Converter* converters_get_first(Converters *converters, ConverterArrayType type)
{
	switch (type){
		case ConverterArrayType::copy:
			if (converters->copy_converters.size() > 0)
				return converters->copy_converters[0];
			break;
		case ConverterArrayType::paste:
			if (converters->paste_converters.size() > 0)
				return converters->paste_converters[0];
			break;
		case ConverterArrayType::display:
			return converters->display_converter;
			break;
		case ConverterArrayType::color_list:
			return converters->color_list_converter;
			break;
	}
	return 0;
}
Converter** converters_get_all_type(Converters *converters, ConverterArrayType type, size_t *size)
{
	switch (type){
		case ConverterArrayType::copy:
			if (converters->copy_converters.size() > 0){
				*size = converters->copy_converters.size();
				return &converters->copy_converters[0];
			}
			break;
		case ConverterArrayType::paste:
			if (converters->paste_converters.size() > 0){
				*size = converters->paste_converters.size();
				return &converters->paste_converters[0];
			}
			break;
		case ConverterArrayType::display:
			*size = 1;
			return &converters->display_converter;
			break;
		case ConverterArrayType::color_list:
			*size = 1;
			return &converters->color_list_converter;
			break;
	}
	return 0;
}
Converter** converters_get_all(Converters *converters, size_t *size)
{
	size_t total_converters = converters->all_converters.size();
	Converter** converter_table = new Converter* [total_converters+1];
	size_t table_i = 0;
	for (list<Converter*>::iterator i=converters->all_converters.begin(); i != converters->all_converters.end(); ++i){
		converter_table[table_i] = *i;
		++table_i;
	}
	if (size) *size = total_converters;
	return converter_table;
}
int converters_reorder(Converters *converters, const char** priority_names, size_t priority_names_size)
{
	Converter* c;
	Converters::ConverterMap used_converters;
	Converters::ConverterMap::iterator used_i;
	converters->all_converters.clear();
	if (priority_names && priority_names_size>0){
		for (size_t i = 0; i < priority_names_size; ++i){
			used_i = used_converters.find( priority_names[i] );
			if (used_i == used_converters.end()){
				if ((c = converters_get(converters, priority_names[i]))){
					converters->all_converters.push_back(c);
					used_converters[c->function_name] = c;
				}
			}
		}
	}
	Converters::ConverterMap::iterator i;
	for (i=converters->converters.begin(); i != converters->converters.end(); ++i){
		used_i = used_converters.find( ((*i).second)->function_name );
		if (used_i == used_converters.end()){
			converters->all_converters.push_back(((*i).second));
			used_converters[((*i).second)->function_name] = ((*i).second);
		}
	}
	return 0;
}
int converters_rebuild_arrays(Converters *converters, ConverterArrayType type)
{
	list<Converter*>::iterator i;
	switch (type){
		case ConverterArrayType::copy:
			converters->copy_converters.clear();
			for (i=converters->all_converters.begin(); i != converters->all_converters.end(); ++i){
				if ((*i)->copy && (*i)->serialize_available){
					converters->copy_converters.push_back(*i);
				}
			}
			return 0;
			break;
		case ConverterArrayType::paste:
			converters->paste_converters.clear();
			for (i=converters->all_converters.begin(); i != converters->all_converters.end(); ++i){
				if ((*i)->paste && (*i)->deserialize_available){
					converters->paste_converters.push_back(*i);
				}
			}
			return 0;
			break;
		default:
			return -1;
	}
	return -1;
}
int converters_set(Converters *converters, Converter* converter, ConverterArrayType type)
{
	switch (type){
		case ConverterArrayType::display:
			converters->display_converter = converter;
			break;
		case ConverterArrayType::color_list:
			converters->color_list_converter = converter;
			break;
		default:
			return -1;
	}
	return 0;
}

bool converter_get_text(const Color &color, ConverterArrayType type, GlobalState *gs, std::string &text)
{
	auto converters = gs->getConverters();
	auto converter = converters_get_first(converters, type);
	if (converter == nullptr) return "";
	ColorObject color_object("", color);
	ConverterSerializePosition position;
	return (converters_color_serialize(converter, &color_object, position, text) == 0);
}
bool converter_get_text(const ColorObject *color_object, ConverterArrayType type, GlobalState *gs, std::string &text)
{
	auto converters = gs->getConverters();
	auto converter = converters_get_first(converters, type);
	if (converter == nullptr) return "";
	ConverterSerializePosition position;
	return (converters_color_serialize(converter, color_object, position, text) == 0);
}
bool converter_get_text(const ColorObject *color_object, Converter *converter, GlobalState *gs, std::string &text)
{
	if (converter == nullptr) return "";
	ConverterSerializePosition position;
	return (converters_color_serialize(converter, color_object, position, text) == 0);
}
bool converter_get_color_object(const char *text, GlobalState* gs, ColorObject** output_color_object)
{
	ColorObject color_object;
	auto converters = gs->getConverters();
	typedef multimap<float, ColorObject*, greater<float> > ValidConverters;
	ValidConverters valid_converters;
	Converter *converter = converters_get_first(converters, ConverterArrayType::display);
	if (converter){
		if (converter->deserialize_available){
			float quality;
			if (converters_color_deserialize(converter, text, &color_object, &quality) == 0){
				if (quality > 0){
					valid_converters.insert(make_pair(quality, color_object.copy()));
				}
			}
		}
	}
	size_t table_size;
	Converter **converter_table;
	if ((converter_table = converters_get_all_type(converters, ConverterArrayType::paste, &table_size))){
		for (uint32_t i = 0; i != table_size; ++i){
			converter = converter_table[i];
			if (converter->deserialize_available){
				float quality;
				if (converters_color_deserialize(converter, text, &color_object, &quality) == 0){
					if (quality > 0){
						valid_converters.insert(make_pair(quality, color_object.copy()));
					}
				}
			}
		}
	}
	bool first = true;
	for (ValidConverters::iterator i = valid_converters.begin(); i != valid_converters.end(); ++i){
		if (first){
			first = false;
			*output_color_object = (*i).second;
		}else{
			(*i).second->release();
		}
	}
	if (first){
		return false;
	}else{
		return true;
	}
}

ConverterSerializePosition::ConverterSerializePosition():
	first(true),
	last(true),
	index(0),
	count(1)
{
}
ConverterSerializePosition::ConverterSerializePosition(size_t count):
	first(true),
	last(count <= 1),
	index(0),
	count(count)
{
}
