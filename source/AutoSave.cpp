/*
 * Copyright (c) 2009-2021, Albertas Vyšniauskas
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

#include "AutoSave.h"
#include "Paths.h"
#include "FileFormat.h"
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <filesystem>
#include <iostream>
void autoSave(ColorList *colorList) {
	using namespace boost::interprocess;
	using namespace std::filesystem;
	try {
		named_mutex mutex(open_or_create, "gpick.autosave");
		scoped_lock<named_mutex> lock(mutex);
		auto fileName = buildConfigPath("autosave.gpa");
		auto fileNameTmp = buildConfigPath("autosave.gpa.tmp");
		auto result = paletteFileSave(fileNameTmp.c_str(), colorList);
		if (!result) {
			std::cerr << "failed to save palette to \"" << fileNameTmp << "\": " << result.error() << std::endl;
		} else {
			std::error_code ec;
			rename(path(fileNameTmp), path(fileName), ec);
			if (ec) {
				std::cerr << "failed to move palette file \"" << fileNameTmp << "\" to \"" << fileName << "\": " << ec << std::endl;
			}
		}
	} catch (const interprocess_exception &e) {
		std::cerr << "failed to acquire interprocess lock: " << e.what() << std::endl;
	}
}
