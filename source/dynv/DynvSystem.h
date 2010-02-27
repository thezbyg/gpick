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

#ifndef DYNVSYSTEM_H_
#define DYNVSYSTEM_H_

#include "DynvHandler.h"

#include <map>
#include <vector>
#include <ostream>
#include <istream>

#include <stdint.h>

struct dynvSystem{
	class dynvKeyCompare{
	public:
		bool operator() (const char* const& x, const char* const& y) const;
	};
	typedef std::map<const char*, struct dynvVariable*, dynvKeyCompare> VariableMap;
	uint32_t refcnt;
	VariableMap variables;
	dynvHandlerMap* handler_map;
};

struct dynvSystem* dynv_system_create(struct dynvHandlerMap* handler_map);
int dynv_system_release(struct dynvSystem* dynv_system);
struct dynvSystem* dynv_system_ref(struct dynvSystem* dynv_system);

struct dynvHandlerMap* dynv_system_get_handler_map(struct dynvSystem* dynv_system);
void dynv_system_set_handler_map(struct dynvSystem* dynv_system, struct dynvHandlerMap* handler_map);

int dynv_system_set(struct dynvSystem* dynv_system, const char* handler_name, const char* variable_name, void* value);
struct dynvVariable* dynv_system_add_empty(struct dynvSystem* dynv_system, struct dynvHandler* handler, const char* variable_name);
void* dynv_system_get(struct dynvSystem* dynv_system, const char* handler_name, const char* variable_name);
void* dynv_system_get_r(struct dynvSystem* dynv_system, const char* handler_name, const char* variable_name, int* error);

int dynv_system_set_array(struct dynvSystem* dynv_system, const char* handler_name, const char* variable_name, void** values, uint32_t count, uint32_t data_size);
void** dynv_system_get_array_r(struct dynvSystem* dynv_system, const char* handler_name, const char* variable_name, uint32_t *count, uint32_t data_size, int* error);

struct dynvVariable* dynv_system_get_var(struct dynvSystem* dynv_system, const char* variable_name);

int dynv_system_remove(struct dynvSystem* dynv_system, const char* variable_name);
int dynv_system_remove_all(struct dynvSystem* dynv_system);

struct dynvSystem* dynv_system_copy(struct dynvSystem* dynv_system);


int dynv_system_serialize(struct dynvSystem* dynv_system, struct dynvIO* io);
int dynv_system_deserialize(struct dynvSystem* dynv_system, dynvHandlerMap::HandlerVec& handler_vec, struct dynvIO* io);


int dynv_set(struct dynvSystem* dynv_system, const char* handler_name, const char* variable_path, const void* value);
void* dynv_get(struct dynvSystem* dynv_system, const char* handler_name, const char* variable_path, int* error);

void** dynv_get_array(struct dynvSystem* dynv_system, const char* handler_name, const char* variable_path, uint32_t *count, int* error);
int dynv_set_array(struct dynvSystem* dynv_system, const char* handler_name, const char* variable_path, const void** values, uint32_t count);

#endif /* DYNVSYSTEM_H_ */
