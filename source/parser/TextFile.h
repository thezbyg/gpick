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

#ifndef GPICK_PARSER_TEXT_FILE_H_
#define GPICK_PARSER_TEXT_FILE_H_

class Color;
#include <cstddef>

namespace text_file_parser {
	class Configuration
	{
		public:
			Configuration();
			bool single_line_c_comments;
			bool single_line_hash_comments;
			bool multi_line_c_comments;
			bool short_hex;
			bool full_hex;
			bool css_rgb;
			bool css_rgba;
			bool float_values;
			bool int_values;
	};
	class TextFile
	{
		public:
			bool parse(const Configuration &configuration);
			virtual ~TextFile();
			virtual void outOfMemory() = 0;
			virtual void syntaxError(size_t start_line, size_t start_column, size_t end_line, size_t end_colunn) = 0;
			virtual size_t read(char *buffer, size_t length) = 0;
			virtual void addColor(const Color &color) = 0;
	};
}

#endif /* GPICK_PARSER_TEXT_FILE_H_ */
