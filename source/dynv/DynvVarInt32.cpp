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

#include "DynvVarInt32.h"
#include "DynvVariable.h"
#include "DynvIO.h"
#include "../Endian.h"

#include <sstream>
using namespace std;

static int dynv_var_int32_create(struct dynvVariable* variable){
	if ((variable->value=new int32_t)){
		return 0;
	}
	return -1;
}

static int dynv_var_int32_destroy(struct dynvVariable* variable){
	if (variable->value){
		delete (int32_t*)variable->value;
		return 0;
	}
	return -1;
}

static int dynv_var_int32_set(struct dynvVariable* variable, void* value, bool deref){
	if (!variable->value) return -1;
	*((int32_t*)variable->value)=*((int32_t*)value);
	return 0;
}

static int dynv_var_int32_get(struct dynvVariable* variable, void** value){
	if (variable->value){
		*value = variable->value;
		return 0;
	}
	return -1;
}

static int dynv_var_int32_serialize(struct dynvVariable* variable, struct dynvIO* io){
	if (!variable->value) return -1;
	uint32_t written;

	uint32_t length=4;
	length=UINT32_TO_LE(length);

	dynv_io_write(io, &length, 4, &written);

	int32_t value=UINT32_TO_LE(*((int32_t*)variable->value));
	if (dynv_io_write(io, &value, 4, &written)==0){
		if (written==4) return 0;
	}
	return -1;
}

static int dynv_var_int32_deserialize(struct dynvVariable* variable, struct dynvIO* io){
	if (!variable->value) return -1;
	uint32_t read;
	int32_t value;
	dynv_io_read(io, &value, 4, &read);

	if (dynv_io_read(io, &value, 4, &read)==0){
		if (read==4){
			*((int32_t*)variable->value)=UINT32_FROM_LE(value);
			return 0;
		}
	}
	return -1;
}

static int serialize_xml(struct dynvVariable* variable, ostream& out){
	if (variable->value){
		out << *(int32_t*)variable->value;
	}
	return 0;
}

static int deserialize_xml(struct dynvVariable* variable, const char *data){
	stringstream ss(stringstream::in);
	ss.str(data);
	int32_t v;
	ss >> v;
	
	*((int32_t*)variable->value) = v;
	
	return 0;
}

struct dynvHandler* dynv_var_int32_new(){
	struct dynvHandler* handler=dynv_handler_create("int32");

	handler->create=dynv_var_int32_create;
	handler->destroy=dynv_var_int32_destroy;
	handler->set=dynv_var_int32_set;
	handler->get=dynv_var_int32_get;
	handler->serialize=dynv_var_int32_serialize;
	handler->deserialize=dynv_var_int32_deserialize;
	
	handler->serialize_xml = serialize_xml;
	handler->deserialize_xml = deserialize_xml;
	
	handler->data_size = sizeof(int32_t*);
	
	return handler;
}
