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

#ifndef DYNVHELPERS_H_
#define DYNVHELPERS_H_

#include "dynv/DynvSystem.h"
#include "Color.h"

#include <stdint.h>
#include <stdbool.h>

int32_t dynv_get_int32_wd(struct dynvSystem* dynv_system, const char *path, int32_t default_value);
float dynv_get_float_wd(struct dynvSystem* dynv_system, const char *path, float default_value);
bool dynv_get_bool_wd(struct dynvSystem* dynv_system, const char *path, bool default_value);
const char* dynv_get_string_wd(struct dynvSystem* dynv_system, const char *path, const char* default_value);
const Color* dynv_get_color_wd(struct dynvSystem* dynv_system, const char *path, const Color* default_value);
Color* dynv_get_color_wdc(struct dynvSystem* dynv_system, const char *path, Color* default_value);
const void* dynv_get_pointer_wd(struct dynvSystem* dynv_system, const char *path, const void* default_value);
void* dynv_get_pointer_wdc(struct dynvSystem* dynv_system, const char *path, void* default_value);

void dynv_set_int32(struct dynvSystem* dynv_system, const char *path, int32_t value);
void dynv_set_float(struct dynvSystem* dynv_system, const char *path, float value);
void dynv_set_bool(struct dynvSystem* dynv_system, const char *path, bool value);
void dynv_set_string(struct dynvSystem* dynv_system, const char *path, const char* value);
void dynv_set_color(struct dynvSystem* dynv_system, const char *path, const Color* value);
void dynv_set_pointer(struct dynvSystem* dynv_system, const char *path, const void* value);

struct dynvSystem* dynv_get_dynv(struct dynvSystem* dynv_system, const char *path);

int32_t* dynv_get_int32_array_wd(struct dynvSystem* dynv_system, const char *path, int32_t *default_value, uint32_t default_count, uint32_t *count);
float* dynv_get_float_array_wd(struct dynvSystem* dynv_system, const char *path, float *default_value, uint32_t default_count, uint32_t *count);
bool* dynv_get_bool_array_wd(struct dynvSystem* dynv_system, const char *path, bool *default_value, uint32_t default_count, uint32_t *count);
const char** dynv_get_string_array_wd(struct dynvSystem* dynv_system, const char *path, const char** default_value, uint32_t default_count, uint32_t *count);
const Color** dynv_get_color_array_wd(struct dynvSystem* dynv_system, const char *path, const Color** default_value, uint32_t default_count, uint32_t *count);

void dynv_set_int32_array(struct dynvSystem* dynv_system, const char *path, int32_t* values, uint32_t count);
void dynv_set_float_array(struct dynvSystem* dynv_system, const char *path, float* values, uint32_t count);
void dynv_set_bool_array(struct dynvSystem* dynv_system, const char *path, bool* values, uint32_t count);
void dynv_set_string_array(struct dynvSystem* dynv_system, const char *path, const char** values, uint32_t count);
void dynv_set_color_array(struct dynvSystem* dynv_system, const char *path, const Color** values, uint32_t count);

#endif /* DYNVHELPERS_H_ */
