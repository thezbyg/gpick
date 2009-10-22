/*
 * Copyright (c) 2009, Albertas Vyšniauskas
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

#include <map>
#include <vector>

#include <stdint.h>

struct dynvHandler;

struct dynvIO{
	int (*write)(struct dynvIO* io, void* data, uint32_t size, uint32_t* data_written);
	int (*read)(struct dynvIO* io, void* data, uint32_t size, uint32_t* data_read);
	int (*seek)(struct dynvIO* io, uint32_t offset, int type, uint32_t* position);
	int (*free)(struct dynvIO* io);
	int (*reset)(struct dynvIO* io);
	
	void* userdata;
};

int dynv_io_write(struct dynvIO* io, void* data, uint32_t size, uint32_t* data_written);
int dynv_io_read(struct dynvIO* io, void* data, uint32_t size, uint32_t* data_read);
int dynv_io_seek(struct dynvIO* io, uint32_t offset, int type, uint32_t* position);
int dynv_io_free(struct dynvIO* io);
int dynv_io_reset(struct dynvIO* io);

class dynvKeyCompare{
public:
	bool operator() (const char* const& x, const char* const& y) const;
};

struct dynvVariable{
	char* name;

	struct dynvHandler* handler;
	void* value;
	uint32_t flags;
};

#define DYNV_VARIABLE_TEMPORARY			1

struct dynvVariable* dynv_variable_create(const char* name, struct dynvHandler* handler);
void dynv_variable_destroy(struct dynvVariable* variable);

void dynv_variable_set_flags(struct dynvVariable* variable, uint32_t flags);

struct dynvHandler{
	char* name;

	int (*set)(struct dynvVariable* variable, void* value);
	int (*create)(struct dynvVariable* variable);
	int (*destroy)(struct dynvVariable* variable);

	int (*get)(struct dynvVariable* variable, void** value);

	int (*serialize)(struct dynvVariable* variable, struct dynvIO* io);
	int (*deserialize)(struct dynvVariable* variable, struct dynvIO* io);

	uint32_t id;
};

struct dynvHandler* dynv_handler_create(const char* name);
void dynv_handler_destroy(struct dynvHandler* handler);

struct dynvHandlerMap{
	typedef std::map<const char*, struct dynvHandler*, dynvKeyCompare> HandlerMap;
	typedef std::vector<struct dynvHandler*> HandlerVec;
	uint32_t refcnt;
	HandlerMap handlers;
};

struct dynvHandlerMap* dynv_handler_map_create();
int dynv_handler_map_release(struct dynvHandlerMap* handler_map);
struct dynvHandlerMap* dynv_handler_map_ref(struct dynvHandlerMap* handler_map);

int dynv_handler_map_add_handler(struct dynvHandlerMap* handler_map, struct dynvHandler* handler);
struct dynvHandler* dynv_handler_map_get_handler(struct dynvHandlerMap* handler_map, const char* handler_name);

int dynv_handler_map_serialize(struct dynvHandlerMap* handler_map, struct dynvIO* io);
int dynv_handler_map_deserialize(struct dynvHandlerMap* handler_map, struct dynvIO* io, dynvHandlerMap::HandlerVec& handler_vec);

struct dynvSystem{
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
struct dynvVariable* dynv_system_add_empty(struct dynvSystem* dynv_system, const char* handler_name, const char* variable_name);
void* dynv_system_get(struct dynvSystem* dynv_system, const char* handler_name, const char* variable_name);

struct dynvVariable* dynv_system_get_var(struct dynvSystem* dynv_system, const char* variable_name);

int dynv_system_remove(struct dynvSystem* dynv_system, const char* variable_name);
int dynv_system_remove_all(struct dynvSystem* dynv_system);

struct dynvSystem* dynv_system_copy(struct dynvSystem* dynv_system);


int dynv_system_serialize(struct dynvSystem* dynv_system, struct dynvIO* io);
int dynv_system_deserialize(struct dynvSystem* dynv_system, dynvHandlerMap::HandlerVec& handler_vec, struct dynvIO* io);

#endif /* DYNVSYSTEM_H_ */
