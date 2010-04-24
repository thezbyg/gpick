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

#include "DynvHelpers.h"

int32_t dynv_get_int32_wd(struct dynvSystem* dynv_system, const char *path, int32_t default_value){
	int error;
	void* r = dynv_get(dynv_system, "int32", path, &error);
	if (error){
		return default_value;
	}else return *(int32_t*)r;
}

float dynv_get_float_wd(struct dynvSystem* dynv_system, const char *path, float default_value){
	int error;
	void* r = dynv_get(dynv_system, "float", path, &error);
	if (error){
		return default_value;
	}else return *(float*)r;
}

bool dynv_get_bool_wd(struct dynvSystem* dynv_system, const char *path, bool default_value){
	int error;
	void* r = dynv_get(dynv_system, "bool", path, &error);
	if (error){
		return default_value;
	}else return *(bool*)r;
}

const char* dynv_get_string_wd(struct dynvSystem* dynv_system, const char *path, const char* default_value){
	int error;
	void* r = dynv_get(dynv_system, "string", path, &error);
	if (error){
		return default_value;
	}else return *(const char**)r;
}

const Color* dynv_get_color_wd(struct dynvSystem* dynv_system, const char *path, const Color* default_value){
	int error;
	void* r = dynv_get(dynv_system, "color", path, &error);
	if (error){
		return default_value;
	}else return *(const Color**)r;
}

Color* dynv_get_color_wdc(struct dynvSystem* dynv_system, const char *path, Color* default_value){
	int error;
	void* r = dynv_get(dynv_system, "color", path, &error);
	if (error){
		return default_value;
	}else return *(Color**)r;
}

const void* dynv_get_pointer_wd(struct dynvSystem* dynv_system, const char *path, const void* default_value){
	int error;
	void* r = dynv_get(dynv_system, "ptr", path, &error);
	if (error){
		return default_value;
	}else return *(const void**)r;
}

void* dynv_get_pointer_wdc(struct dynvSystem* dynv_system, const char *path, void* default_value){
	int error;
	void* r = dynv_get(dynv_system, "ptr", path, &error);
	if (error){
		return default_value;
	}else return *(void**)r;
}



void dynv_set_int32(struct dynvSystem* dynv_system, const char *path, int32_t value){
	dynv_set(dynv_system, "int32", path, &value);
}

void dynv_set_float(struct dynvSystem* dynv_system, const char *path, float value){
	dynv_set(dynv_system, "float", path, &value);
}

void dynv_set_bool(struct dynvSystem* dynv_system, const char *path, bool value){
	dynv_set(dynv_system, "bool", path, &value);
}

void dynv_set_string(struct dynvSystem* dynv_system, const char *path, const char* value){
	dynv_set(dynv_system, "string", path, &value);
}

void dynv_set_color(struct dynvSystem* dynv_system, const char *path, const Color* value){
	dynv_set(dynv_system, "color", path, &value);
}

void dynv_set_pointer(struct dynvSystem* dynv_system, const char *path, const void* value){
	dynv_set(dynv_system, "ptr", path, &value);
}

struct dynvSystem* dynv_get_dynv(struct dynvSystem* dynv_system, const char *path){
	int error;
	void* r = dynv_get(dynv_system, "dynv", path, &error);
	if (error){
		struct dynvHandlerMap* handler_map = dynv_system_get_handler_map(dynv_system);
		struct dynvSystem* dynv = dynv_system_create(handler_map);
		dynv_handler_map_release(handler_map);
		dynv_set(dynv_system, "dynv", path, dynv);
		return dynv;
	}else return (struct dynvSystem*)r;
}


int32_t* dynv_get_int32_array_wd(struct dynvSystem* dynv_system, const char *path, int32_t* default_value, uint32_t default_count, uint32_t *count){
	int error;
	void** r = dynv_get_array(dynv_system, "int32", path, count, &error);
	if (error){
		if (count) *count = default_count;
		return default_value;
	}else return (int32_t*)r;
}

float* dynv_get_float_array_wd(struct dynvSystem* dynv_system, const char *path, float *default_value, uint32_t default_count, uint32_t *count){
	int error;
	void** r = dynv_get_array(dynv_system, "float", path, count, &error);
	if (error){
		if (count) *count = default_count;
		return default_value;
	}else return (float*)r;
}

bool* dynv_get_bool_array_wd(struct dynvSystem* dynv_system, const char *path, bool *default_value, uint32_t default_count, uint32_t *count){
	int error;
	void** r = dynv_get_array(dynv_system, "bool", path, count, &error);
	if (error){
		if (count) *count = default_count;
		return default_value;
	}else return (bool*)r;
}

const char** dynv_get_string_array_wd(struct dynvSystem* dynv_system, const char *path, const char** default_value, uint32_t default_count, uint32_t *count){
	int error;
	void** r = dynv_get_array(dynv_system, "string", path, count, &error);
	if (error){
		if (count) *count = default_count;
		return default_value;
	}else return (const char**)r;
}

const Color** dynv_get_color_array_wd(struct dynvSystem* dynv_system, const char *path, const Color** default_value, uint32_t default_count, uint32_t *count){
	int error;
	void** r = dynv_get_array(dynv_system, "color", path, count, &error);
	if (error){
		if (count) *count = default_count;
		return default_value;
	}else return (const Color**)r;
}

void dynv_set_int32_array(struct dynvSystem* dynv_system, const char *path, int32_t* values, uint32_t count){
	dynv_set_array(dynv_system, "int32", path, (const void**)values, count);
}

void dynv_set_float_array(struct dynvSystem* dynv_system, const char *path, float* values, uint32_t count){
	dynv_set_array(dynv_system, "float", path, (const void**)values, count);
}

void dynv_set_bool_array(struct dynvSystem* dynv_system, const char *path, bool* values, uint32_t count){
	dynv_set_array(dynv_system, "bool", path, (const void**)values, count);
}

void dynv_set_string_array(struct dynvSystem* dynv_system, const char *path, const char** values, uint32_t count){
	dynv_set_array(dynv_system, "string", path, (const void**)values, count);
}

void dynv_set_color_array(struct dynvSystem* dynv_system, const char *path, const Color** values, uint32_t count){
	dynv_set_array(dynv_system, "color", path, (const void**)values, count);
}
