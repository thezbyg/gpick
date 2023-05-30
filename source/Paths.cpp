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
#include <filesystem>
#include <optional>
#include <vector>
#include <boost/predef.h>
#include <stdexcept>
#include <iostream>
#if BOOST_OS_WINDOWS != 0
#include <windows.h>
#elif BOOST_OS_MACOS != 0
#include <mach-o/dyld.h>
#else
#include <unistd.h>
#endif
namespace fs = std::filesystem;
using path = fs::path;
struct PathException: std::runtime_error {
	PathException(const char *message):
		std::runtime_error(message) {
	}
};
#if BOOST_OS_LINUX != 0
static std::string getExecutablePath() {
	std::vector<char> buffer;
	buffer.resize(4096);
	while (1) {
		int count = ::readlink("/proc/self/exe", &buffer.front(), buffer.size());
		if (count < 0)
			throw PathException("could not read executable path");
		if (static_cast<size_t>(count) < buffer.size())
			return std::string(buffer.begin(), buffer.begin() + count);
		buffer.resize(buffer.size() * 2);
	}
}
#elif BOOST_OS_WINDOWS != 0
static std::string getExecutablePath() {
	std::vector<TCHAR> buffer;
	buffer.resize(4096);
	size_t length;
	while (1) {
		length = GetModuleFileName(nullptr, &buffer.front(), buffer.size());
		if (length == 0 && GetLastError() != ERROR_INSUFFICIENT_BUFFER)
			throw PathException("could not read executable path");
		if (length < buffer.size())
			return std::string(buffer.begin(), buffer.begin() + length);
		buffer.resize(buffer.size() * 2);
	}
}
#elif BOOST_OS_MACOS != 0
static std::string getExecutablePath() {
	uint32_t bufferSize = 4096;
	std::vector<char> buffer;
	buffer.resize(bufferSize);
	while (1) {
		if (_NSGetExecutablePath(buffer.data(), &bufferSize) == 0)
			return buffer.data();
		buffer.resize(bufferSize);
	}
}
#else
#error getExecutablePath not implemented for current OS
#endif
static path &getUserConfigPath() {
	static std::optional<path> configPath;
	if (configPath)
		return *configPath;
	configPath = path(g_get_user_config_dir());
	return *configPath;
}
static bool validateDataPath(const path &path) {
	try {
		std::error_code ec;
		if (!fs::is_directory(fs::status(path, ec)))
			return false;
		if (!fs::is_regular_file(fs::status(path / ".gpick-data-directory", ec)))
			return false;
		return true;
	} catch (const fs::filesystem_error &) {
		return false;
	}
}
static bool getRelativeDataPath(std::optional<path> &dataPath) {
	try {
		path testPath;
		if (validateDataPath(testPath = (path(getExecutablePath()).parent_path() / "share" / "gpick"))) {
			dataPath = testPath;
			return true;
		}
		if (validateDataPath(testPath = (path(getExecutablePath()).parent_path().parent_path() / "share" / "gpick"))) {
			dataPath = testPath;
			return true;
		}
		return false;
	} catch (const PathException &) {
		return false;
	} catch (const fs::filesystem_error &) {
		return false;
	}
}
static path &getDataPath() {
	static std::optional<path> dataPath;
	if (dataPath)
		return *dataPath;
	path testPath;
#ifdef GPICK_DEV_BUILD
	if (getRelativeDataPath(dataPath))
		return *dataPath;
#endif
	if (validateDataPath(testPath = (path(g_get_user_data_dir()) / "gpick")))
		return *(dataPath = testPath);
	auto dataPaths = g_get_system_data_dirs();
	for (size_t i = 0; dataPaths[i]; ++i) {
		if (validateDataPath(testPath = (path(dataPaths[i]) / "gpick")))
			return *(dataPath = testPath);
	}
#ifndef GPICK_DEV_BUILD
	if (getRelativeDataPath(dataPath))
		return *dataPath;
#endif
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
