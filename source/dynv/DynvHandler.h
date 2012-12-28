/*
 * Copyright (c) 2009-2012, Albertas Vyšniauskas
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

#ifndef DYNVHANDLER_H_
#define DYNVHANDLER_H_

#include <map>
#include <vector>
#include <ostream>
#include <istream>

#include <stdint.h>
#ifndef _MSC_VER
#include <stdbool.h>
#endif

struct dynvIO;

struct dynvHandler{
	char* name;

	int (*set)(struct dynvVariable* variable, void* value, bool deref);
	int (*create)(struct dynvVariable* variable);
	int (*destroy)(struct dynvVariable* variable);

	int (*get)(struct dynvVariable* variable, void** value, bool *deref);

	int (*serialize)(struct dynvVariable* variable, struct dynvIO* io);
	int (*deserialize)(struct dynvVariable* variable, struct dynvIO* io);

	int (*serialize_xml)(struct dynvVariable* variable, std::ostream& out);
	int (*deserialize_xml)(struct dynvVariable* variable, const char* data);

	uint32_t id;
	uint32_t data_size;
};

struct dynvHandler* dynv_handler_create(const char* name);
void dynv_handler_destroy(struct dynvHandler* handler);

struct dynvHandlerMap{
	class dynvKeyCompare{
	public:
		bool operator() (const char* const& x, const char* const& y) const;
	};
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

#endif /* DYNVHANDLER_H_ */
