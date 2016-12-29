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

#ifndef GPICK_IMPORT_EXPORT_H_
#define GPICK_IMPORT_EXPORT_H_

class ColorList;
class Converter;
class Converters;
class GlobalState;
namespace text_file_parser {
class Configuration;
}
enum class FileType
{
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
class ImportExport
{
	public:
		enum class Error
		{
			none,
			could_not_open_file,
			file_read_error,
			file_write_error,
			no_colors_imported,
			parsing_failed,
		};
		enum class ItemSize
		{
			small,
			medium,
			big,
			controllable,
		};
		enum class Background
		{
			none,
			white,
			gray,
			black,
			first_color,
			last_color,
			controllable,
		};
		ImportExport(ColorList *color_list, const char* filename, GlobalState *gs);
		void setConverter(Converter *converter);
		void setConverters(Converters *converters);
		void setItemSize(ItemSize item_size);
		void setItemSize(const char *item_size);
		void setBackground(Background background);
		void setBackground(const char *background);
		void setIncludeColorNames(bool include_color_names);
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
	private:
		ColorList *m_color_list;
		Converter *m_converter;
		Converters *m_converters;
		const char* m_filename;
		ItemSize m_item_size;
		Background m_background;
		GlobalState *m_gs;
		bool m_include_color_names;
		Error m_last_error;
};

#endif /* GPICK_IMPORT_EXPORT_H_ */
