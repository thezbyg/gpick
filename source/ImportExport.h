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

#pragma once
#include <string>
struct ColorList;
struct Converter;
struct Converters;
struct GlobalState;
namespace text_file_parser {
struct Configuration;
}
enum struct FileType {
	gpa,
	gpl,
	ase,
	txt,
	mtl,
	css,
	html,
	rgbtxt,
	unknown,
};
struct ImportExport {
	enum struct Error {
		none,
		couldNotOpenFile,
		fileReadError,
		fileWriteError,
		noColorsImported,
		parsingFailed,
	};
	enum struct ItemSize {
		small,
		medium,
		big,
		controllable,
	};
	enum struct Background {
		none,
		white,
		gray,
		black,
		firstColor,
		lastColor,
		controllable,
	};
	ImportExport(ColorList &colorList, const char *filename, GlobalState &gs);
	void setConverter(Converter *converter);
	void setConverters(Converters *converters);
	void setItemSize(ItemSize itemSize);
	void setItemSize(const char *itemSize);
	void setBackground(Background background);
	void setBackground(const char *background);
	void setIncludeColorNames(bool includeColorNames);
	bool exportGPL();
	bool importGPL();
	bool exportASE();
	bool importASE();
	bool exportCSS();
	bool importTXT();
	bool exportTXT();
	bool importGPA();
	bool exportGPA();
	bool exportMTL();
	bool exportHTML();
	bool importTextFile(const text_file_parser::Configuration &configuration);
	bool importRGBTXT();
	bool importType(FileType type);
	bool exportType(FileType type);
	Error getLastError() const;
	static FileType getFileType(const char *filename);
	static FileType getFileTypeByExtension(const char *extension);
	static FileType getFileTypeByContent(const char *filename);
	void fixFileExtension(const char *selectedFilter);
	const std::string &getFilename() const;
private:
	ColorList &m_colorList;
	Converter *m_converter;
	Converters *m_converters;
	std::string m_filename;
	ItemSize m_itemSize;
	Background m_background;
	GlobalState &m_gs;
	bool m_includeColorNames;
	Error m_lastError;
};
