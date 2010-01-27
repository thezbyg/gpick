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

#include "DynvSystem.h"
#include "DynvVariable.h"
#include "DynvIO.h"
#include "DynvXml.h"
#include "DynvVarDynv.h"
#include "../Endian.h"

#include <iostream>
using namespace std;

static int create(struct dynvVariable* variable){
	variable->ptr_value = 0;
	return 0;
}

static int destroy(struct dynvVariable* variable){
	if (variable->ptr_value){
		dynv_system_release((struct dynvSystem*)variable->ptr_value);
		return 0;
	}
	return -1;
}

static int set(struct dynvVariable* variable, void* value, bool deref){
	if (variable->ptr_value) dynv_system_release((struct dynvSystem*)variable->ptr_value);
	variable->ptr_value = dynv_system_ref((struct dynvSystem*)value);
	return 0;
}

static int get(struct dynvVariable* variable, void** value){
	if (variable->ptr_value){
		*value= dynv_system_ref((struct dynvSystem*)variable->ptr_value);
		return 0;
	}
	return -1;
}

static int serialize_xml(struct dynvVariable* variable, ostream& out){
	if (variable->ptr_value){
		out << endl;
		dynv_xml_serialize((struct dynvSystem*)variable->ptr_value, out);
	}
	return 0;
}

struct dynvHandler* dynv_var_dynv_new(){
	struct dynvHandler* handler=dynv_handler_create("dynv");

	handler->create=create;
	handler->destroy=destroy;
	handler->set=set;
	handler->get=get;
	//handler->serialize=serialize;
	//handler->deserialize=deserialize;

	handler->serialize_xml = serialize_xml;

	handler->data_size = sizeof(struct dynvSystem*);
	return handler;
}
