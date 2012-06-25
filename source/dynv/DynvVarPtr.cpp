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

#include "DynvVarPtr.h"
#include "DynvVariable.h"
#include "DynvIO.h"
#include "../Endian.h"

static int dynv_var_ptr_create(struct dynvVariable* variable){
	variable->ptr_value = 0;
	return 0;
}

static int dynv_var_ptr_destroy(struct dynvVariable* variable){
	return 0;
}

static int dynv_var_ptr_set(struct dynvVariable* variable, void* value, bool deref){
	variable->ptr_value = *(void**)value;
	return 0;
}

static int dynv_var_ptr_get(struct dynvVariable* variable, void** value, bool *deref){
	*value = &variable->ptr_value;
	return 0;
}

struct dynvHandler* dynv_var_ptr_new(){
	struct dynvHandler* handler=dynv_handler_create("ptr");

	handler->create=dynv_var_ptr_create;
	handler->destroy=dynv_var_ptr_destroy;
	handler->set=dynv_var_ptr_set;
	handler->get=dynv_var_ptr_get;
	handler->serialize=0;
	handler->deserialize=0;

	handler->data_size = sizeof(void*);

	return handler;
}
