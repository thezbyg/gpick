/*
 * Copyright (c) 2009-2010, Albertas Vyšniauskas
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

#ifndef CONVERTER_H_
#define CONVERTER_H_

#include "dynv/DynvSystem.h"
#include <stdbool.h>
#include <stdint.h>

class Converters;

enum ConvertersArrayType{
	CONVERTERS_ARRAY_TYPE_COPY,
	CONVERTERS_ARRAY_TYPE_PASTE,
	CONVERTERS_ARRAY_TYPE_DISPLAY,
	CONVERTERS_ARRAY_TYPE_COLOR_LIST,
};

typedef struct Converter{
	char* function_name;
	char* human_readable;
	bool copy, serialize_available;
	bool paste, deserialize_available;
}Converter;

Converters* converters_init(struct dynvSystem* params);
int converters_term(Converters *converters);

Converter* converters_get(Converters *converters, const char* name);
//Converter** converters_get_all(Converters *converters, const char** priority_names, uint32_t priority_names_size);

int converters_set(Converters *converters, Converter* converter, ConvertersArrayType type);

Converter* converters_get_first(Converters *converters, ConvertersArrayType type);
Converter** converters_get_all_type(Converters *converters, ConvertersArrayType type, uint32_t *size);
Converter** converters_get_all(Converters *converters, uint32_t *size);

int converters_color_serialize(Converters* converters, const char* function, struct ColorObject* color_object, char** result);
int converters_color_deserialize(Converters* converters, const char* function, char* text, struct ColorObject* color_object, float* conversion_quality);

int converters_rebuild_arrays(Converters *converters, ConvertersArrayType type);
int converters_reorder(Converters *converters, const char** priority_names, uint32_t priority_names_size);

#endif /* CONVERTER_H_ */
