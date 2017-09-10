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

#include "GlobalState.h"
#include "Paths.h"
#include "ScreenReader.h"
#include "Converters.h"
#include "Converter.h"
#include "Random.h"
#include "color_names/ColorNames.h"
#include "Sampler.h"
#include "ColorList.h"
#include "layout/Layout.h"
#include "layout/Layouts.h"
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
#include "lua/Script.h"
#include "lua/Extensions.h"
#include "lua/Callbacks.h"
#include <stdlib.h>
#include <glib/gstdio.h>
extern "C"{
#include <lualib.h>
#include <lauxlib.h>
}
#include <fstream>
#include <iostream>
using namespace std;

struct GlobalState::Impl
{
	GlobalState *m_decl;
	ColorNames *m_color_names;
	Sampler *m_sampler;
	ScreenReader *m_screen_reader;
	ColorList *m_color_list;
	dynvSystem *m_settings;
	lua::Script m_script;
	Random *m_random;
	Converters m_converters;
	layout::Layouts m_layouts;
	lua::Callbacks m_callbacks;
	transformation::Chain *m_transformation_chain;
	GtkWidget *m_status_bar;
	ColorSource *m_color_source;
	Impl(GlobalState *decl):
		m_decl(decl),
		m_color_names(nullptr),
		m_sampler(nullptr),
		m_screen_reader(nullptr),
		m_color_list(nullptr),
		m_settings(nullptr),
		m_random(nullptr),
		m_transformation_chain(nullptr),
		m_status_bar(nullptr),
		m_color_source(nullptr)
	{
	}
	~Impl()
	{
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
			g_mkdir(config_dir, 0);
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
		dynvSystem *params = dynv_get_dynv(m_settings, "gpick");
		color_names_load(m_color_names, params);
		dynv_system_release(params);
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
	string gcharToString(gchar *value)
	{
		string result(value);
		g_free(value);
		return result;
	}
	bool initializeLua()
	{
		lua_State *L = m_script;
		lua::registerAll(L, *m_decl);
		vector<string> paths;
		paths.push_back(gcharToString(build_filename("")));
		paths.push_back(gcharToString(build_config_path("")));
		m_script.setPaths(paths);
		bool result = m_script.load("init");
		if (!result){
			cerr << m_script.getLastError() << endl;
		}
		return result;
	}
	bool loadConverters()
	{
		char** source_array;
		uint32_t source_array_size;
		if ((source_array = (char**)dynv_get_string_array_wd(m_settings, "gpick.converters.names", 0, 0, &source_array_size))){
			bool *copy_array, *paste_array;
			uint32_t copy_array_size = 0, paste_array_size = 0;
			copy_array = dynv_get_bool_array_wd(m_settings, "gpick.converters.copy", 0, 0, &copy_array_size);
			paste_array = dynv_get_bool_array_wd(m_settings, "gpick.converters.paste", 0, 0, &paste_array_size);
			gsize source_array_i = 0;
			Converter *converter;
			if (copy_array_size > 0 || paste_array_size > 0){
				while (source_array_i < source_array_size){
					converter = m_converters.byName(source_array[source_array_i]);
					if (converter){
						if (source_array_i < copy_array_size)
							converter->copy(converter->hasSerialize() && copy_array[source_array_i]);
						if (source_array_i < paste_array_size)
							converter->paste(converter->hasDeserialize() && paste_array[source_array_i]);
					}
					++source_array_i;
				}
			}else{
				while (source_array_i < source_array_size){
					converter = m_converters.byName(source_array[source_array_i]);
					if (converter){
						converter->copy(converter->hasSerialize());
						converter->paste(converter->hasDeserialize());
					}
					++source_array_i;
				}
			}
			if (copy_array) delete [] copy_array;
			if (paste_array) delete [] paste_array;
			m_converters.reorder((const char**)source_array, source_array_size);
			if (source_array) delete [] source_array;
		}else{
			//Initialize default values
			const char* name_array[] = {
				"color_web_hex",
				"color_css_rgb",
				"color_css_hsl",
			};
			source_array_size = sizeof(name_array) / sizeof(name_array[0]);
			gsize source_array_i = 0;
			Converter* converter;
			while (source_array_i < source_array_size){
				converter = m_converters.byName(name_array[source_array_i]);
				if (converter){
					converter->copy(converter->hasSerialize());
					converter->paste(converter->hasDeserialize());
				}
				++source_array_i;
			}
			m_converters.reorder(name_array, source_array_size);
		}
		m_converters.rebuildCopyPasteArrays();
		m_converters.display(dynv_get_string_wd(m_settings, "gpick.converters.display", "color_web_hex"));
		m_converters.colorList(dynv_get_string_wd(m_settings, "gpick.converters.color_list", "color_web_hex"));
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
		loadSettings();
		loadColorNames();
		createColorList();
		initializeLua();
		loadConverters();
		loadTransformationChain();
		return true;
	}
};

GlobalState::GlobalState()
{
	m_impl = make_unique<Impl>(this);
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
lua::Script &GlobalState::script()
{
	return m_impl->m_script;
}
lua::Callbacks &GlobalState::callbacks()
{
	return m_impl->m_callbacks;
}
Random *GlobalState::getRandom()
{
	return m_impl->m_random;
}
Converters &GlobalState::converters()
{
	return m_impl->m_converters;
}
layout::Layouts &GlobalState::layouts()
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
