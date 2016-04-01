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

#ifndef GPICK_CONVERTER_H_
#define GPICK_CONVERTER_H_

class Converters;
struct lua_State;
struct dynvSystem;
class ColorObject;
class GlobalState;
struct Color;
#include <string>
#ifndef _MSC_VER
#include <stdbool.h>
#endif
#include <stdint.h>

enum class ConverterArrayType{
	copy,
	paste,
	display,
	color_list,
};

class Converter
{
	public:
		char* function_name;
		char* human_readable;
		bool copy, serialize_available;
		bool paste, deserialize_available;
		Converters *converters;
};

Converters* converters_init(lua_State *lua, dynvSystem *settings);
int converters_term(Converters *converters);
Converter* converters_get(Converters *converters, const char* name);
int converters_set(Converters *converters, Converter* converter, ConverterArrayType type);
Converter* converters_get_first(Converters *converters, ConverterArrayType type);
Converter** converters_get_all_type(Converters *converters, ConverterArrayType type, size_t *size);
Converter** converters_get_all(Converters *converters, size_t *size);

class ConverterSerializePosition
{
	public:
		ConverterSerializePosition();
		ConverterSerializePosition(size_t count);
		bool first;
		bool last;
		size_t index;
		size_t count;
};

int converters_color_serialize(Converters* converters, const char* function, const ColorObject* color_object, const ConverterSerializePosition &position, std::string& result);
int converters_color_serialize(Converter* converter, const ColorObject* color_object, const ConverterSerializePosition &position, std::string& result);
int converters_color_deserialize(Converters* converters, const char* function, const char* text, ColorObject* color_object, float* conversion_quality);
int converters_color_deserialize(Converter *converter, const char* text, ColorObject *color_object, float* conversion_quality);
int converters_rebuild_arrays(Converters *converters, ConverterArrayType type);
int converters_reorder(Converters *converters, const char** priority_names, size_t priority_names_size);

bool converter_get_text(const Color &color, ConverterArrayType type, GlobalState *gs, std::string &text);
bool converter_get_text(const ColorObject *color_object, ConverterArrayType type, GlobalState *gs, std::string &text);
bool converter_get_text(const ColorObject *color_object, Converter *converter, GlobalState *gs, std::string &text);

bool converter_get_color_object(const char *text, GlobalState* gs, ColorObject** output_color_object);

#endif /* GPICK_CONVERTER_H_ */
