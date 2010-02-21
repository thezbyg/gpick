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

#include "Converter.h"
#include "DynvHelpers.h"

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
	~Converters();
};

Converters::~Converters(){
	Converters::ConverterMap::iterator i;

	for (i=converters.begin(); i!=converters.end(); ++i){
		g_free(((*i).second)->human_readable);
		g_free(((*i).second)->function_name);
		delete ((*i).second);
	}
	converters.clear();
}

/*static int get_human_readable_name(lua_State *L, const char* function, char** human_readable){
	if (L==NULL) return -1;

	size_t st;
	int status;
	int stack_top = lua_gettop(L);

	lua_getglobal(L, "gpick");
	int gpick_namespace = lua_gettop(L);
	if (lua_type(L, -1)!=LUA_TNIL){

		lua_pushstring(L, "converters");
		lua_gettable(L, gpick_namespace);
		int converters_table = lua_gettop(L);
		if (lua_type(L, -1)!=LUA_TNIL){

			lua_pushstring(L, function);
			lua_gettable(L, converters_table);
			if (lua_type(L, -1)!=LUA_TNIL){

				lua_pushstring(L, "human_readable");
				lua_gettable(L, -2);

				*human_readable = strdup(lua_tostring(L, -1));
				lua_settop(L, stack_top);
				return 0;
			}
		}
	}
	lua_settop(L, stack_top);
	return -1;
}*/

int converters_color_deserialize(Converters* converters, const char* function, char* text, struct ColorObject* color_object, float* conversion_quality){
	lua_State* L = converters->L;

	int status;
	int stack_top = lua_gettop(L);

	lua_getglobal(L, "gpick");
	int gpick_namespace = lua_gettop(L);
	if (lua_type(L, -1)!=LUA_TNIL){

		lua_pushstring(L, "color_deserialize");
		lua_gettable(L, gpick_namespace);
		if (lua_type(L, -1) != LUA_TNIL){

			lua_pushstring(L, function);
			lua_pushstring(L, text);
			lua_pushcolorobject (L, color_object);

			status=lua_pcall(L, 3, 1, 0);
			if (status==0){
				if (lua_type(L, -1)==LUA_TNUMBER){
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

int converters_color_serialize(Converters* converters, const char* function, struct ColorObject* color_object, char** result){
	lua_State* L = converters->L;

	size_t st;
	int status;
	int stack_top = lua_gettop(L);

	lua_getglobal(L, "gpick");
	int gpick_namespace = lua_gettop(L);
	if (lua_type(L, -1)!=LUA_TNIL){

		lua_pushstring(L, "color_serialize");
		lua_gettable(L, gpick_namespace);
		if (lua_type(L, -1) != LUA_TNIL){

			lua_pushstring(L, function);
			//lua_gettable(L, -2);
			lua_pushcolorobject (L, color_object);

			status=lua_pcall(L, 2, 1, 0);
			if (status==0){
				if (lua_type(L, -1)==LUA_TSTRING){
					const char* converted = luaL_checklstring(L, -1, &st);
					*result = g_strdup(converted);
					lua_settop(L, stack_top);
					return 0;
				}else{
					cerr<<"gpick.color_serialize: returned not a string value \""<<function<<"\""<<endl;
				}
			}else{
				cerr<<"gpick.color_serialize: "<<lua_tostring (L, -1)<<endl;
			}
		}else{
			cerr<<"gpick.color_serialize: no such function \""<<function<<"\""<<endl;
		}
	}

	lua_settop(L, stack_top);
	return -1;
}

Converters* converters_init(struct dynvSystem* params){
	Converters *converters = new Converters;


	lua_State* L = (lua_State*)dynv_get_pointer_wd(params, "lua_State", 0);
	if (L==NULL) return 0;

	converters->L = L;
	converters->display_converter = 0;

	int stack_top = lua_gettop(L);
	lua_getglobal(L, "gpick");
	int gpick_namespace = lua_gettop(L);
	if (lua_type(L, -1)!=LUA_TNIL){

		lua_pushstring(L, "converters");
		lua_gettable(L, gpick_namespace);
		int converters_table = lua_gettop(L);

		lua_pushnil(L);
		while (lua_next(L, converters_table) != 0){
			if (lua_type(L, -2) == LUA_TSTRING){
				Converter *converter = new Converter;
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
			lua_pop(L, 1);      //pop value from stack, but leave key
		}
	}
	lua_settop(L, stack_top);

	dynv_set_pointer(params, "Converters", converters);

	return converters;
}

int converters_term(Converters *converters){
	delete converters;
	return 0;
}

Converter* converters_get(Converters *converters, const char* name){
	Converters::ConverterMap::iterator i;
	i=converters->converters.find( name);
	if (i!=converters->converters.end()){
		return (*i).second;
	}else{
		return 0;
	}
}

Converter* converters_get_first(Converters *converters, ConvertersArrayType type){
	switch (type){
	case CONVERTERS_ARRAY_TYPE_COPY:
		if (converters->copy_converters.size()>0)
			return converters->copy_converters[0];
		break;
	case CONVERTERS_ARRAY_TYPE_PASTE:
		if (converters->paste_converters.size()>0)
			return converters->paste_converters[0];
		break;
	case CONVERTERS_ARRAY_TYPE_DISPLAY:
		return converters->display_converter;
		break;
	case CONVERTERS_ARRAY_TYPE_COLOR_LIST:
		return converters->color_list_converter;
		break;
	}
	return 0;
}

Converter** converters_get_all_type(Converters *converters, ConvertersArrayType type, uint32_t *size){
	switch (type){
	case CONVERTERS_ARRAY_TYPE_COPY:
		if (converters->copy_converters.size()>0){
			*size = converters->copy_converters.size();
			return &converters->copy_converters[0];
		}
		break;
	case CONVERTERS_ARRAY_TYPE_PASTE:
		if (converters->paste_converters.size()>0){
			*size = converters->paste_converters.size();
			return &converters->paste_converters[0];
		}
		break;
	case CONVERTERS_ARRAY_TYPE_DISPLAY:
		*size = 1;
		return &converters->display_converter;
		break;
	case CONVERTERS_ARRAY_TYPE_COLOR_LIST:
		*size = 1;
		return &converters->color_list_converter;
		break;
	}
	return 0;
}

Converter** converters_get_all(Converters *converters, uint32_t *size){
	uint32_t total_converters = converters->all_converters.size();
	Converter** converter_table = new Converter* [total_converters+1];
	uint32_t table_i = 0;

	for (list<Converter*>::iterator i=converters->all_converters.begin(); i!=converters->all_converters.end(); ++i){
		converter_table[table_i] = *i;
		++table_i;
	}

	if (size) *size = total_converters;
	return converter_table;
}

int converters_reorder(Converters *converters, const char** priority_names, uint32_t priority_names_size){

	//uint32_t total_converters = converters->converters.size();
	//Converter** converter_table = new Converter* [total_converters+1];
	//uint32_t table_i = 0;
	Converter* c;
	//converter_table[total_converters] = 0;

	Converters::ConverterMap used_converters;
	Converters::ConverterMap::iterator used_i;

	converters->all_converters.clear();

	if (priority_names && priority_names_size>0){
		for (uint32_t i=0; i<priority_names_size; ++i){
			used_i = used_converters.find( priority_names[i] );
			if (used_i==used_converters.end()){
				if ((c = converters_get(converters, priority_names[i]))){
					//converter_table[table_i++] = c;
					converters->all_converters.push_back(c);
					used_converters[c->function_name] = c;
				}
			}
		}
	}

	Converters::ConverterMap::iterator i;
	for (i=converters->converters.begin(); i!=converters->converters.end(); ++i){
		used_i = used_converters.find( ((*i).second)->function_name );
		if (used_i==used_converters.end()){
			//converter_table[table_i++] = ((*i).second);
			converters->all_converters.push_back(((*i).second));
			used_converters[((*i).second)->function_name] = ((*i).second);
		}
	}

	return 0;
}

int converters_rebuild_arrays(Converters *converters, ConvertersArrayType type){
	list<Converter*>::iterator i;

	switch (type){
	case CONVERTERS_ARRAY_TYPE_COPY:
		converters->copy_converters.clear();
		for (i=converters->all_converters.begin(); i!=converters->all_converters.end(); ++i){
			if ((*i)->copy && (*i)->serialize_available){
				converters->copy_converters.push_back(*i);
			}
		}
		return 0;
		break;
	case CONVERTERS_ARRAY_TYPE_PASTE:
		converters->paste_converters.clear();
		for (i=converters->all_converters.begin(); i!=converters->all_converters.end(); ++i){
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



int converters_set(Converters *converters, Converter* converter, ConvertersArrayType type){
	switch (type){
	case CONVERTERS_ARRAY_TYPE_DISPLAY:
		converters->display_converter = converter;
		break;
	case CONVERTERS_ARRAY_TYPE_COLOR_LIST:
		converters->color_list_converter = converter;
		break;
	default:
		return -1;
	}
	return 0;
}
