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
#include "dynv/Map.h"
#include "lua/Script.h"
#include "lua/Extensions.h"
#include "lua/Callbacks.h"
#include <boost/filesystem.hpp>
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
	dynv::Map m_settings;
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
	}
	bool writeSettings() {
		auto configFile = buildConfigPath("settings.xml");
		std::ofstream settingsFile(configFile.c_str());
		if (!settingsFile.is_open()){
			return false;
		}
		if (!m_settings.serializeXml(settingsFile))
			return false;
		settingsFile.close();
		return settingsFile.good();
	}
	bool loadSettings() {
		auto configFile = buildConfigPath("settings.xml");
		std::ifstream settingsFile(configFile.c_str());
		if (!settingsFile.is_open()){
			return false;
		}
		if (!m_settings.deserializeXml(settingsFile)) {
			return false;
		}
		settingsFile.close();
		return true;
	}
	// Creates configuration directory if it doesn't exist
	void checkConfigurationDirectory() {
		namespace fs = boost::filesystem;
		auto configPath = fs::path(buildConfigPath());
		boost::system::error_code ec;
		fs::create_directory(configPath, ec);
	}
	// Check if user has user_init.lua file, if not, then create empty file
	void checkUserInitFile() {
		namespace fs = boost::filesystem;
		auto userInitFilePath = fs::path(buildConfigPath("user_init.lua"));
		if (!fs::exists(fs::status(userInitFilePath)))
			return;
		auto pathString = userInitFilePath.string();
		std::ofstream newFile(pathString.c_str());
		if (!newFile.is_open()) {
			return;
		}
		newFile.close();
	}
	bool loadColorNames()
	{
		if (m_color_names != nullptr) return false;
		m_color_names = color_names_new();
		auto options = m_settings.getOrCreateMap("gpick");
		color_names_load(m_color_names, *options);
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
		m_color_list = color_list_new();
		return true;
	}
	bool initializeLua()
	{
		lua_State *L = m_script;
		lua::registerAll(L, *m_decl);
		std::vector<std::string> paths;
		paths.push_back(buildFilename());
		paths.push_back(buildConfigPath());
		m_script.setPaths(paths);
		bool result = m_script.load("init");
		if (!result){
			std::cerr << "Lua load error: " << m_script.getLastError() << "\n";
		}
		return result;
	}
	bool loadConverters()
	{
		auto converters = m_settings.getOrCreateMap("gpick.converters");
		if (converters->size() == 0) {
			const char* names[] = {
				"color_web_hex",
				"color_css_rgb",
				"color_css_hsl",
			};
			for (size_t i = 0; i != sizeof(names) / sizeof(names[0]); i++) {
				auto converter = m_converters.byName(names[i]);
				if (!converter)
					continue;
				converter->copy(converter->hasSerialize());
				converter->paste(converter->hasDeserialize());
			}
			m_converters.reorder(names, sizeof(names) / sizeof(names[0]));
		} else {
			auto names = converters->getStrings("names");
			auto copy = converters->getBools("copy");
			auto paste = converters->getBools("paste");
			for (size_t i = 0, end = names.size(); i != end; i++) {
				auto converter = m_converters.byName(names[i].c_str());
				if (!converter)
					continue;
				if (i < copy.size())
					converter->copy(converter->hasSerialize() && copy[i]);
				if (i < paste.size())
					converter->paste(converter->hasDeserialize() && paste[i]);
			}
			m_converters.reorder(names);
		}
		m_converters.rebuildCopyPasteArrays();
		m_converters.display(m_settings.getString("gpick.converters.display", "color_web_hex"));
		m_converters.colorList(m_settings.getString("gpick.converters.color_list", "color_web_hex"));
		return true;
	}
	bool loadTransformationChain()
	{
		if (m_transformation_chain != nullptr) return false;
		transformation::Chain *chain = new transformation::Chain();
		chain->setEnabled(m_settings.getBool("gpick.transformations.enabled", false));
		auto items = m_settings.getMaps("gpick.transformations.items");
		for (auto values: items) {
			if (!values)
				continue;
			auto name = values->getString("name", "");
			if (name.empty())
				continue;
			auto transformation = transformation::Factory::create(name);
			if (!transformation)
				continue;
			transformation->deserialize(*values);
			chain->add(std::move(transformation));
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
dynv::Map &GlobalState::settings()
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
