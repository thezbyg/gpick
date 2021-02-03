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

#include "Paths.h"
#include <glib.h>
#include <boost/filesystem.hpp>
#include <boost/optional.hpp>
#include <boost/predef.h>
#include <exception>
namespace fs = boost::filesystem;
using path = fs::path;
struct PathException : std::runtime_error {
	PathException(const char *message):
		std::runtime_error(message) {
	}
};
#if BOOST_OS_LINUX
static std::string getExecutablePath() {
	std::vector<char> buffer;
	buffer.resize(4096);
	while (1) {
		int count = ::readlink("/proc/self/exe", &buffer.front(), buffer.size());
		if (count < 0)
			throw PathException("could not read executable path");
		if ((size_t)count < buffer.size())
			return std::string(buffer.begin(), buffer.begin() + count);
		buffer.resize(buffer.size() * 2);
	}
}
#else
static std::string getExecutablePath() {
	return "";
}
#endif
static path &getUserConfigPath() {
	static boost::optional<path> configPath;
	if (configPath)
		return *configPath;
	configPath = path(g_get_user_config_dir());
	return *configPath;
}
static path &getDataPath() {
	static boost::optional<path> dataPath;
	if (dataPath)
		return *dataPath;
	path testPath;
	try {
		if (fs::is_directory(fs::status(testPath = (path(getExecutablePath()).remove_filename() / "share" / "gpick"))))
			return *(dataPath = testPath);
	} catch (const PathException &) {
	}
	if (fs::is_directory(fs::status(testPath = (path(g_get_user_data_dir()) / "gpick"))))
		return *(dataPath = testPath);
	auto dataPaths = g_get_system_data_dirs();
	for (size_t i = 0; dataPaths[i]; ++i) {
		if (fs::is_directory(fs::status(testPath = (path(dataPaths[i]) / "gpick"))))
			return *(dataPath = testPath);
	}
	dataPath = path();
	return *dataPath;
}
std::string buildFilename(const char *filename) {
	if (filename)
		return (getDataPath() / filename).string();
	else
		return getDataPath().string();
}
std::string buildConfigPath(const char *filename) {
	if (filename)
		return (getUserConfigPath() / "gpick" / filename).string();
	else
		return (getUserConfigPath() / "gpick").string();
}
