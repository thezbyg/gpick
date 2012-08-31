/*
 * Copyright (c) 2009-2012, Albertas Vyšniauskas
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

#include "GlobalState.h"
#include "GlobalStateStruct.h"
#include "Paths.h"
#include "ScreenReader.h"
#include "Converter.h"

#include "layout/LuaBindings.h"
#include "layout/Layout.h"

#include "transformation/Chain.h"
#include "transformation/Factory.h"

#include "dynv/DynvMemoryIO.h"
#include "dynv/DynvVarString.h"
#include "dynv/DynvVarInt32.h"
#include "dynv/DynvVarColor.h"
#include "dynv/DynvVarPtr.h"
#include "dynv/DynvVarFloat.h"
#include "dynv/DynvVarDynv.h"
#include "dynv/DynvVarBool.h"
#include "dynv/DynvXml.h"

#include "DynvHelpers.h"

#include <stdlib.h>
#include <glib/gstdio.h>

#include <fstream>
#include <iostream>
using namespace std;

GlobalState* global_state_create(){
	GlobalState *gs = new GlobalState;
	gs->params = 0;
	gs->lua = 0;
	gs->loaded_levels = GlobalStateLevel(0);
	gs->random = 0;
	return gs;
}

int global_state_destroy(GlobalState* gs){
	delete gs;
	return 0;
}

int global_state_term(GlobalState *gs){

	//write settings to file
	gchar* config_file = build_config_path("settings.xml");
	ofstream params_file(config_file);
	if (params_file.is_open()){
		params_file << "<?xml version=\"1.0\" encoding='UTF-8'?><root>" << endl;
		dynv_xml_serialize(gs->params, params_file);
		params_file << "</root>" << endl;
		params_file.close();
	}
	g_free(config_file);

	//destroy converter system
	converters_term((Converters*)dynv_get_pointer_wd(gs->params, "Converters", 0));

	//destroy layout system
	layout::layouts_term((layout::Layouts*)dynv_get_pointer_wd(gs->params, "Layouts", 0));

	//destroy transformation chain
	transformation::Chain *chain = reinterpret_cast<transformation::Chain*>(dynv_get_pointer_wdc(gs->params, "TransformationChain", 0));
	delete chain;

	//destroy color list, random generator and other systems
	color_list_destroy(gs->colors);
	random_destroy(gs->random);
	color_names_destroy(gs->color_names);
	sampler_destroy(gs->sampler);
	screen_reader_destroy(gs->screen_reader);
	dynv_system_release(gs->params);
	lua_close(gs->lua);

	return 0;
}

int global_state_init(GlobalState *gs, GlobalStateLevel level){

	//Create configuration directory if it doesn't exit
	struct stat st;
	gchar* config_dir = build_config_path(NULL);
	if (g_stat(config_dir, &st)!=0){
		g_mkdir(config_dir, S_IRWXU);
	}
	g_free(config_dir);


	if ((level & GLOBALSTATE_SCRIPTING) && !(gs->loaded_levels & GLOBALSTATE_SCRIPTING)){
		//check if user has user_init.lua file, if not, then create empty file
		gchar *user_init_file = build_config_path("user_init.lua");
		FILE *user_init = fopen(user_init_file, "r");
		if (user_init){
			fclose(user_init);
		}else{
			user_init = fopen(user_init_file, "w");
			if (user_init){

				fclose(user_init);
			}
		}
		g_free(user_init_file);
	}

	if ((level & GLOBALSTATE_OTHER) && !(gs->loaded_levels & GLOBALSTATE_OTHER)){
		gs->screen_reader = screen_reader_new();

		gs->sampler = sampler_new(gs->screen_reader);
		//create and seed random generator
		gs->random = random_new("SHR3");
		gulong seed_value=time(0)|1;
		random_seed(gs->random, &seed_value);
	}

	if ((level & GLOBALSTATE_COLOR_NAMES) && !(gs->loaded_levels & GLOBALSTATE_COLOR_NAMES)){
		//create and load color names
		gs->color_names = color_names_new();
		gchar* tmp;
		color_names_load_from_file(gs->color_names, tmp=build_filename("colors.txt"));
		g_free(tmp);
		color_names_load_from_file(gs->color_names, tmp=build_filename("colors0.txt"));
		g_free(tmp);

		gs->loaded_levels = GlobalStateLevel(gs->loaded_levels | GLOBALSTATE_COLOR_NAMES);
	}

	if ((level & GLOBALSTATE_CONFIGURATION) && !(gs->loaded_levels & GLOBALSTATE_CONFIGURATION)){
		//create dynamic parameter system
		struct dynvHandlerMap* handler_map = dynv_handler_map_create();
		dynv_handler_map_add_handler(handler_map, dynv_var_string_new());
		dynv_handler_map_add_handler(handler_map, dynv_var_int32_new());
		dynv_handler_map_add_handler(handler_map, dynv_var_color_new());
		dynv_handler_map_add_handler(handler_map, dynv_var_ptr_new());
		dynv_handler_map_add_handler(handler_map, dynv_var_float_new());
		dynv_handler_map_add_handler(handler_map, dynv_var_dynv_new());
		dynv_handler_map_add_handler(handler_map, dynv_var_bool_new());
		gs->params = dynv_system_create(handler_map);
		dynv_handler_map_release(handler_map);

		gchar* config_file = build_config_path("settings.xml");
		ifstream params_file(config_file);
		if (params_file.is_open()){
			dynv_xml_deserialize(gs->params, params_file);
			params_file.close();
		}
		g_free(config_file);

		gs->loaded_levels = GlobalStateLevel(gs->loaded_levels | GLOBALSTATE_CONFIGURATION);
	}

	if ((level & GLOBALSTATE_COLOR_LIST) && !(gs->loaded_levels & GLOBALSTATE_COLOR_LIST)){
		//create color list / callbacks must be defined elsewhere
		struct dynvHandlerMap* handler_map = dynv_system_get_handler_map(gs->params);

		gs->colors = color_list_new(handler_map);

		dynv_handler_map_release(handler_map);

		gs->loaded_levels = GlobalStateLevel(gs->loaded_levels | GLOBALSTATE_COLOR_LIST);
	}


	if ((level & GLOBALSTATE_SCRIPTING) && !(gs->loaded_levels & GLOBALSTATE_SCRIPTING)){
		//create and load lua state
		lua_State *L= luaL_newstate();
		luaL_openlibs(L);

		int status;
		char *tmp;
		lua_ext_colors_openlib(L);
		layout::lua_ext_layout_openlib(L);

		gchar* lua_root_path = build_filename("?.lua");
		gchar* lua_user_path = build_config_path("?.lua");

		gchar* lua_path = g_strjoin(";", lua_root_path, lua_user_path, (void*)0);

		lua_getglobal(L, "package");
		lua_pushstring(L, "path");
		lua_pushstring(L, lua_path);
		lua_settable(L, -3);
		lua_pop(L, 1);

		g_free(lua_path);
		g_free(lua_root_path);
		g_free(lua_user_path);

		tmp = build_filename("init.lua");
		status = luaL_loadfile(L, tmp) || lua_pcall(L, 0, 0, 0);
		if (status) {
			cerr<<"init script load failed: "<<lua_tostring (L, -1)<<endl;
		}
		g_free(tmp);

		gs->lua = L;
		dynv_set_pointer(gs->params, "lua_State", L);

		gs->loaded_levels = GlobalStateLevel(gs->loaded_levels | GLOBALSTATE_SCRIPTING);
	}

	if ((level & GLOBALSTATE_CONVERTERS) && !(gs->loaded_levels & GLOBALSTATE_CONVERTERS)){
		//create converter system
		Converters* converters = converters_init(gs->params);

		char** source_array;
		uint32_t source_array_size;

		if ((source_array = (char**)dynv_get_string_array_wd(gs->params, "gpick.converters.names", 0, 0, &source_array_size))){
			bool* copy_array;
			uint32_t copy_array_size=0;
			bool* paste_array;
			uint32_t paste_array_size=0;

			copy_array = dynv_get_bool_array_wd(gs->params, "gpick.converters.copy", 0, 0, &copy_array_size);
			paste_array = dynv_get_bool_array_wd(gs->params, "gpick.converters.paste", 0, 0, &paste_array_size);

			gsize source_array_i = 0;
			Converter* converter;

			if (copy_array_size>0 || paste_array_size>0){

				while (source_array_i<source_array_size){
					converter = converters_get(converters, source_array[source_array_i]);
					if (converter){
						if (source_array_i<copy_array_size) converter->copy = converter->serialize_available & copy_array[source_array_i];
						if (source_array_i<paste_array_size) converter->paste = converter->deserialize_available & paste_array[source_array_i];
					}
					++source_array_i;
				}
			}else{
				while (source_array_i<source_array_size){
					converter = converters_get(converters, source_array[source_array_i]);
					if (converter){
						converter->copy = converter->serialize_available;
						converter->paste = converter->deserialize_available;
					}
					++source_array_i;
				}
			}

			if (copy_array) delete [] copy_array;
			if (paste_array) delete [] paste_array;

			converters_reorder(converters, (const char**)source_array, source_array_size);

			if (source_array) delete [] source_array;

		}else{
			//Initialize default values

			const char* name_array[]={
				"color_web_hex",
				"color_css_rgb",
				"color_css_hsl",
			};

			source_array_size = sizeof(name_array)/sizeof(name_array[0]);
			gsize source_array_i = 0;
			Converter* converter;

			while (source_array_i<source_array_size){
				converter = converters_get(converters, name_array[source_array_i]);
				if (converter){
					converter->copy = converter->serialize_available;
					converter->paste = converter->deserialize_available;
				}
				++source_array_i;
			}

			converters_reorder(converters, name_array, source_array_size);
		}

		converters_rebuild_arrays(converters, CONVERTERS_ARRAY_TYPE_COPY);
		converters_rebuild_arrays(converters, CONVERTERS_ARRAY_TYPE_PASTE);

		converters_set(converters, converters_get(converters, dynv_get_string_wd(gs->params, "gpick.converters.display", "color_web_hex")), CONVERTERS_ARRAY_TYPE_DISPLAY);
		converters_set(converters, converters_get(converters, dynv_get_string_wd(gs->params, "gpick.converters.color_list", "color_web_hex")), CONVERTERS_ARRAY_TYPE_COLOR_LIST);

		gs->loaded_levels = GlobalStateLevel(gs->loaded_levels | GLOBALSTATE_CONVERTERS);
	}

	if ((level & GLOBALSTATE_TRANSFORMATIONS) && !(gs->loaded_levels & GLOBALSTATE_TRANSFORMATIONS)){
		transformation::Chain *chain = new transformation::Chain();
		dynv_set_pointer(gs->params, "TransformationChain", chain);
		chain->setEnabled(dynv_get_bool_wd(gs->params, "gpick.transformations.enabled", false));

		struct dynvSystem** config_array;
		uint32_t config_size;

		if ((config_array = (struct dynvSystem**)dynv_get_dynv_array_wd(gs->params, "gpick.transformations.items", 0, 0, &config_size))){
			for (uint32_t i = 0; i != config_size; i++){
				const char *name = dynv_get_string_wd(config_array[i], "name", 0);
				if (name){
					boost::shared_ptr<transformation::Transformation> tran = transformation::Factory::create(name);
					if (tran){
						tran->deserialize(config_array[i]);
						chain->add(tran);
					}
				}
			}

			delete [] config_array;
		}

		gs->loaded_levels = GlobalStateLevel(gs->loaded_levels | GLOBALSTATE_TRANSFORMATIONS);
	}

	if ((level & GLOBALSTATE_OTHER) && !(gs->loaded_levels & GLOBALSTATE_OTHER)){
		//create layout system
		layout::Layouts* layout = layout::layouts_init(gs->params);
		layout = 0;
	}

	if (level & GLOBALSTATE_OTHER){
		gs->loaded_levels = GlobalStateLevel(gs->loaded_levels | GLOBALSTATE_OTHER);
	}

	return 0;
}

