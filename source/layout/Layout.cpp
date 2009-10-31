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

#include "Layout.h"
#include "LuaBindings.h"
#include "../LuaExt.h"

#include <string.h>
#include <stdlib.h>

#include <fstream>
#include <string>
#include <sstream>
#include <map>
#include <list>
#include <vector>
#include <iostream>
using namespace std;

namespace layout{

class LayoutKeyCompare{
public:
	bool operator() (const char* const& x, const char* const& y) const {
		return strcmp(x,y)<0;
	};
};

class Layouts{
public:
	typedef std::map<const char*, Layout*, LayoutKeyCompare> LayoutMap;
	LayoutMap layouts;

	vector<Layout*> all_layouts;

	lua_State *L;
	~Layouts();
};

Layouts::~Layouts(){
	Layouts::LayoutMap::iterator i;

	for (i=layouts.begin(); i!=layouts.end(); ++i){
		delete ((*i).second);
	}
	layouts.clear();
}

Layouts* layouts_init(struct dynvSystem* params){
	Layouts *layouts = new Layouts;
	
	lua_State* L=(lua_State*)dynv_system_get(params, "ptr", "lua_State");
	if (L==NULL) return 0;
	
	layouts->L = L;

	int status;
	int stack_top = lua_gettop(L);
	lua_getglobal(L, "gpick");
	int gpick_namespace = lua_gettop(L);
	if (lua_type(L, -1)!=LUA_TNIL){
		
		lua_pushstring(L, "layouts");
		lua_gettable(L, gpick_namespace);
		int layouts_table = lua_gettop(L);
		
		lua_pushstring(L, "layouts_get");
		lua_gettable(L, gpick_namespace);
		if (lua_type(L, -1) != LUA_TNIL){
			
			if ((status=lua_pcall(L, 0, 1, 0))==0){
				if (lua_type(L, -1)==LUA_TTABLE){
					size_t st;
					int table_index = lua_gettop(L);
					
					for (int i=1;;i++){
						lua_pushinteger(L, i);
						lua_gettable(L, table_index);
						if (lua_isnil(L, -1)) break;
						
						lua_pushstring(L, lua_tostring(L, -1));		//duplicate, because lua_gettable replaces stack top
						lua_gettable(L, layouts_table);

						lua_pushstring(L, "human_readable");
						lua_gettable(L, -2);
						
						Layout *layout = new Layout;
						layout->human_readable = g_strdup(lua_tostring(L, -1));
						layout->name = g_strdup(lua_tostring(L, -3));
						layouts->layouts[layout->name] = layout;
						
						layouts->all_layouts.push_back(layout);
						
						//cout<<layout->name<<endl;
						
						lua_pop(L, 3);
					}
					
				}
			}else{
				cerr<<"layouts_get: "<<lua_tostring (L, -1)<<endl;
			}
		}
	}
	lua_settop(L, stack_top);
	
	dynv_system_set(params, "ptr", "Layouts", layouts);
	
	return layouts;
}

int layouts_term(Layouts *layouts){
	delete layouts;
	return 0;
}

Layout** layouts_get_all(Layouts *layouts, uint32_t *size){
	*size = layouts->layouts.size();
	return &layouts->layouts[0];
}

Box* layouts_get(Layouts *layouts, const char* name){
	Layouts::LayoutMap::iterator i;
	i=layouts->layouts.find(name);
	if (i!=layouts->layouts.end()){
		//layout name matched, build layout
		
		lua_State* L = layouts->L;
		
		int status;
		int stack_top = lua_gettop(L);
		lua_getglobal(L, "gpick");
		int gpick_namespace = lua_gettop(L);
		if (lua_type(L, -1)!=LUA_TNIL){
			
			lua_pushstring(L, "layouts");
			lua_gettable(L, gpick_namespace);
			int layouts_table = lua_gettop(L);
			
			lua_pushstring(L, name);
			lua_gettable(L, layouts_table);
			
			lua_pushstring(L, "build");
			lua_gettable(L, -2);
			
			if (!lua_isnil(L, -1)){
				
				if ((status=lua_pcall(L, 0, 1, 0))==0){
					
					Box* box = lua_checklbox(L, -1);
					
					lua_settop(L, stack_top);
					return box;
				}else{
					cerr<<"layouts.build: "<<lua_tostring (L, -1)<<endl;
				}
			}			
		}
		lua_settop(L, stack_top);
		return 0;
	}else{
		return 0;
	}	
}

}

