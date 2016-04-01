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

#include "GlobalState.h"
#include "Paths.h"
#include "ScreenReader.h"
#include "Converter.h"
#include "Random.h"
#include "color_names/DownloadNameFile.h"
#include "color_names/ColorNames.h"
#include "Sampler.h"
#include "ColorList.h"
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
extern "C"{
#include <lualib.h>
#include <lauxlib.h>
}
#include <fstream>
#include <iostream>
using namespace std;

class GlobalState::Impl
{
	public:
		ColorNames *m_color_names;
		Sampler *m_sampler;
		ScreenReader *m_screen_reader;
		ColorList *m_color_list;
		dynvSystem *m_settings;
		lua_State *m_lua;
		Random *m_random;
		Converters *m_converters;
		layout::Layouts *m_layouts;
		transformation::Chain *m_transformation_chain;
		GtkWidget *m_status_bar;
		ColorSource *m_color_source;
		Impl():
			m_color_names(nullptr),
			m_sampler(nullptr),
			m_screen_reader(nullptr),
			m_color_list(nullptr),
			m_settings(nullptr),
			m_lua(nullptr),
			m_random(nullptr),
			m_converters(nullptr),
			m_layouts(nullptr),
			m_transformation_chain(nullptr),
			m_status_bar(nullptr),
			m_color_source(nullptr)
		{
		}
		~Impl()
		{
			if (m_converters != nullptr)
				converters_term(m_converters);
			if (m_layouts != nullptr)
				layout::layouts_term(m_layouts);
			if (m_transformation_chain != nullptr)
				delete m_transformation_chain;
			if (m_color_list != nullptr)
				color_list_destroy(m_color_list);
			if (m_random != nullptr)
				random_destroy(m_random);
			if (m_color_names != nullptr)
				color_names_destroy(m_color_names);
			if (m_sampler != nullptr)
				sampler_destroy(m_sampler);
			if (m_screen_reader != nullptr)
				screen_reader_destroy(m_screen_reader);
			if (m_settings != nullptr)
				dynv_system_release(m_settings);
			if (m_lua)
				lua_close(m_lua);
		}
		bool writeSettings()
		{
			gchar* config_file = build_config_path("settings.xml");
			ofstream settings_file(config_file);
			if (!settings_file.is_open()){
				g_free(config_file);
				return false;
			}
			settings_file << "<?xml version=\"1.0\" encoding='UTF-8'?><root>" << endl;
			dynv_xml_serialize(m_settings, settings_file);
			settings_file << "</root>" << endl;
			settings_file.close();
			g_free(config_file);
			return true;
		}
		bool loadSettings()
		{
			if (m_settings != nullptr) return false;
			struct dynvHandlerMap* handler_map = dynv_handler_map_create();
			dynv_handler_map_add_handler(handler_map, dynv_var_string_new());
			dynv_handler_map_add_handler(handler_map, dynv_var_int32_new());
			dynv_handler_map_add_handler(handler_map, dynv_var_color_new());
			dynv_handler_map_add_handler(handler_map, dynv_var_ptr_new());
			dynv_handler_map_add_handler(handler_map, dynv_var_float_new());
			dynv_handler_map_add_handler(handler_map, dynv_var_dynv_new());
			dynv_handler_map_add_handler(handler_map, dynv_var_bool_new());
			m_settings = dynv_system_create(handler_map);
			dynv_handler_map_release(handler_map);
			gchar* config_file = build_config_path("settings.xml");
			ifstream settings_file(config_file);
			if (!settings_file.is_open()){
				g_free(config_file);
				return false;
			}
			dynv_xml_deserialize(m_settings, settings_file);
			settings_file.close();
			g_free(config_file);
			return true;
		}
		bool checkConfigurationDirectory()
		{
			//create configuration directory if it doesn't exist
			GStatBuf st;
			gchar* config_dir = build_config_path(nullptr);
			if (g_stat(config_dir, &st) != 0){
#ifndef _MSC_VER
				g_mkdir(config_dir, S_IRWXU);
#else
				g_mkdir(config_dir, nullptr);
#endif
			}
			g_free(config_dir);
			return true;
		}
		bool checkUserInitFile()
		{
			//check if user has user_init.lua file, if not, then create empty file
			gchar *user_init_file = build_config_path("user_init.lua");
			ifstream f(user_init_file);
			if (f.is_open()){
				f.close();
				g_free(user_init_file);
				return true;
			}
			ofstream new_file(user_init_file);
			if (!f.is_open()){
				g_free(user_init_file);
				return false;
			}
			new_file.close();
			g_free(user_init_file);
			return true;
		}
		bool loadColorNames()
		{
			if (m_color_names != nullptr) return false;
			m_color_names = color_names_new();
			gchar* tmp;
			if (color_names_load_from_file(m_color_names, tmp = build_filename("colors.txt")) != 0){
				g_free(tmp);
				if (color_names_load_from_file(m_color_names, tmp = build_config_path("colors.txt")) != 0){
					download_name_file(tmp);
					color_names_load_from_file(m_color_names, tmp);
				}
			}
			g_free(tmp);
			color_names_load_from_file(m_color_names, tmp = build_filename("colors0.txt"));
			g_free(tmp);
			return true;
		}
		bool initializeRandomGenerator()
		{
			m_random = random_new("SHR3");
			size_t seed_value = time(0) | 1;
			random_seed(m_random, &seed_value);
			return true;
		}
		bool createColorList()
		{
			if (m_color_list != nullptr) return false;
			//create color list / callbacks must be defined elsewhere
			struct dynvHandlerMap* handler_map = dynv_system_get_handler_map(m_settings);
			m_color_list = color_list_new(handler_map);
			dynv_handler_map_release(handler_map);
			return true;
		}
		bool initializeLua()
		{
			lua_State *L = luaL_newstate();
			luaL_openlibs(L);
			int status;
			char *tmp;
			lua_ext_colors_openlib(L);
			layout::lua_ext_layout_openlib(L);
			gchar* lua_root_path = build_filename("?.lua");
			gchar* lua_user_path = build_config_path("?.lua");
			gchar* lua_path = g_strjoin(";", lua_root_path, lua_user_path, nullptr);
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
				cerr << "init script load failed: " << lua_tostring(L, -1) << endl;
			}
			g_free(tmp);
			m_lua = L;
			return true;
		}
		bool loadConverters()
		{
			if (m_converters != nullptr) return false;
			Converters *converters = converters_init(m_lua, m_settings);
			char** source_array;
			uint32_t source_array_size;
			if ((source_array = (char**)dynv_get_string_array_wd(m_settings, "gpick.converters.names", 0, 0, &source_array_size))){
				bool* copy_array;
				uint32_t copy_array_size=0;
				bool* paste_array;
				uint32_t paste_array_size=0;
				copy_array = dynv_get_bool_array_wd(m_settings, "gpick.converters.copy", 0, 0, &copy_array_size);
				paste_array = dynv_get_bool_array_wd(m_settings, "gpick.converters.paste", 0, 0, &paste_array_size);
				gsize source_array_i = 0;
				Converter *converter;
				if (copy_array_size > 0 || paste_array_size > 0){
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
				while (source_array_i < source_array_size){
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
			converters_set(converters, converters_get(converters, dynv_get_string_wd(m_settings, "gpick.converters.display", "color_web_hex")), CONVERTERS_ARRAY_TYPE_DISPLAY);
			converters_set(converters, converters_get(converters, dynv_get_string_wd(m_settings, "gpick.converters.color_list", "color_web_hex")), CONVERTERS_ARRAY_TYPE_COLOR_LIST);
			m_converters = converters;
			return true;
		}
		bool loadLayouts()
		{
			if (m_layouts != nullptr) return false;
			m_layouts = layout::layouts_init(m_lua, m_settings);
			return true;
		}
		bool loadTransformationChain()
		{
			if (m_transformation_chain != nullptr) return false;
			transformation::Chain *chain = new transformation::Chain();
			chain->setEnabled(dynv_get_bool_wd(m_settings, "gpick.transformations.enabled", false));
			struct dynvSystem** config_array;
			uint32_t config_size;
			if ((config_array = (struct dynvSystem**)dynv_get_dynv_array_wd(m_settings, "gpick.transformations.items", 0, 0, &config_size))){
				for (uint32_t i = 0; i != config_size; i++){
					const char *name = dynv_get_string_wd(config_array[i], "name", 0);
					if (name){
						boost::shared_ptr<transformation::Transformation> tran = transformation::Factory::create(name);
						if (tran){
							tran->deserialize(config_array[i]);
							chain->add(tran);
						}
					}
					dynv_system_release(config_array[i]);
				}
				delete [] config_array;
			}
			m_transformation_chain = chain;
			return true;
		}
		bool loadAll()
		{
			checkConfigurationDirectory();
			checkUserInitFile();
			m_screen_reader = screen_reader_new();
			m_sampler = sampler_new(m_screen_reader);
			initializeRandomGenerator();
			loadColorNames();
			loadSettings();
			createColorList();
			initializeLua();
			loadConverters();
			loadLayouts();
			loadTransformationChain();
			return true;
		}
};

GlobalState::GlobalState()
{
	m_impl = make_unique<Impl>();
}
GlobalState::~GlobalState()
{
}
bool GlobalState::loadSettings()
{
	return m_impl->loadSettings();
}
bool GlobalState::loadAll()
{
	return m_impl->loadAll();
}
bool GlobalState::writeSettings()
{
	return m_impl->writeSettings();
}
ColorNames *GlobalState::getColorNames()
{
	return m_impl->m_color_names;
}
Sampler *GlobalState::getSampler()
{
	return m_impl->m_sampler;
}
ScreenReader *GlobalState::getScreenReader()
{
	return m_impl->m_screen_reader;
}
ColorList *GlobalState::getColorList()
{
	return m_impl->m_color_list;
}
dynvSystem *GlobalState::getSettings()
{
	return m_impl->m_settings;
}
lua_State *GlobalState::getLua()
{
	return m_impl->m_lua;
}
Random *GlobalState::getRandom()
{
	return m_impl->m_random;
}
Converters *GlobalState::getConverters()
{
	return m_impl->m_converters;
}
layout::Layouts *GlobalState::getLayouts()
{
	return m_impl->m_layouts;
}
transformation::Chain *GlobalState::getTransformationChain()
{
	return m_impl->m_transformation_chain;
}
GtkWidget *GlobalState::getStatusBar()
{
	return m_impl->m_status_bar;
}
void GlobalState::setStatusBar(GtkWidget *status_bar)
{
	m_impl->m_status_bar = status_bar;
}
ColorSource *GlobalState::getCurrentColorSource()
{
	return m_impl->m_color_source;
}
void GlobalState::setCurrentColorSource(ColorSource *color_source)
{
	m_impl->m_color_source = color_source;
}

