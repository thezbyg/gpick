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

#include "DynvVarBool.h"
#include "DynvVariable.h"
#include "DynvIO.h"
#include "../Endian.h"

#include <string.h>
#include <stdlib.h>

#include <iostream>
using namespace std;

static int create(struct dynvVariable* variable){
	variable->bool_value = false;
	return 0;
}

static int destroy(struct dynvVariable* variable){
	return 0;
}

static int set(struct dynvVariable* variable, void* value, bool deref){
	variable->bool_value = *(bool*)value;
	return 0;
}

static int get(struct dynvVariable* variable, void** value){
	*value = &variable->bool_value;
	return 0;
}

static int serialize_xml(struct dynvVariable* variable, ostream& out){
	if (variable->bool_value){
		out << "true";
	}else{
		out << "false";
	}
	return 0;
}

static int deserialize_xml(struct dynvVariable* variable, const char *data){
	if (strcmp(data, "true")==0){
		*(bool*)&variable->bool_value = true;
	}else{
		*(bool*)&variable->bool_value = false;
	}
	return 0;
}

struct dynvHandler* dynv_var_bool_new(){
	struct dynvHandler* handler=dynv_handler_create("bool");

	handler->create=create;
	handler->destroy=destroy;
	handler->set=set;
	handler->get=get;
	//handler->serialize=serialize;
	//handler->deserialize=deserialize;

	handler->serialize_xml = serialize_xml;
	handler->deserialize_xml = deserialize_xml;

	handler->data_size = sizeof(bool);

	return handler;
}
