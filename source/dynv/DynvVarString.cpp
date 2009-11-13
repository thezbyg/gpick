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

#include "DynvVarString.h"
#include "DynvVariable.h"
#include "DynvIO.h"
#include "../Endian.h"
#include <string.h>

using namespace std;

static int dynv_var_string_create(struct dynvVariable* variable){
	variable->value=0;
	return -1;
}

static int dynv_var_string_destroy(struct dynvVariable* variable){
	if (variable->value){
		delete [] (char*)variable->value;
		return 0;
	}
	return -1;
}

static int dynv_var_string_set(struct dynvVariable* variable, void* value, bool deref){
	if (variable->value){
		delete [] (char*)variable->value;
	}
	if (deref) value = *(void**)value;
	
	uint32_t len=strlen((char*)value)+1;
	variable->value=new char [len];
	memcpy(variable->value, value, len);
	return 0;
}

static int dynv_var_string_get(struct dynvVariable* variable, void** value){
	if (variable->value){
		*value=variable->value;
		return 0;
	}
	return -1;
}

static int dynv_var_string_serialize(struct dynvVariable* variable, struct dynvIO* io){
	if (!variable->value) return -1;
	uint32_t written;
	uint32_t length=strlen((char*)variable->value);
	uint32_t length_le=UINT32_TO_LE(length);

	if (dynv_io_write(io, &length_le, 4, &written)==0){
		if (written!=4) return -1;
	}else return -1;

	if (dynv_io_write(io, variable->value, length, &written)==0){
		if (written!=length) return -1;
	}else return -1;

	return 0;
}

static int dynv_var_string_deserialize(struct dynvVariable* variable, struct dynvIO* io){
	if (variable->value){
		delete [] (char*)variable->value;
		variable->value=0;
	}
	uint32_t read;
	uint32_t length;

	if (dynv_io_read(io, &length, 4, &read)==0){
		if (read!=4) return -1;
	}else return -1;

	length=UINT32_FROM_LE(length);

	variable->value=new char [length+1];

	if (dynv_io_read(io, variable->value, length, &read)==0){
		if (read!=length) return -1;
	}else return -1;

	((char*)variable->value)[length]=0;

	return 0;
}

static int serialize_xml(struct dynvVariable* variable, ostream& out){
	if (variable->value){
		out << (char*)variable->value;
	}
	return 0;
}

static int deserialize_xml(struct dynvVariable* variable, const char *data){
	if (variable->value){
		delete [] (char*)variable->value;
		variable->value=0;
	}
	uint32_t len=strlen(data)+1;
	variable->value=new char [len];
	memcpy(variable->value, data, len);
	return 0;
}

struct dynvHandler* dynv_var_string_new(){
	struct dynvHandler* handler=dynv_handler_create("string");

	handler->create=dynv_var_string_create;
	handler->destroy=dynv_var_string_destroy;
	handler->set=dynv_var_string_set;
	handler->get=dynv_var_string_get;
	handler->serialize=dynv_var_string_serialize;
	handler->deserialize=dynv_var_string_deserialize;
	
	handler->serialize_xml=serialize_xml;
	handler->deserialize_xml=deserialize_xml;
	
	handler->data_size = sizeof(char*);
	
	return handler;
}
