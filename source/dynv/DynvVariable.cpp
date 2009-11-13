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

#include "DynvVariable.h"
#include "DynvHandler.h"

#include <string.h>
#include <stdlib.h>

struct dynvVariable* dynv_variable_create(const char* name, struct dynvHandler* handler){
	struct dynvVariable* variable = new struct dynvVariable;
	if (name){
		variable->name = strdup(name);
	}else{
		variable->name = NULL;
	}
	variable->handler = handler;
	variable->value = NULL;
	variable->next = NULL;
	variable->prev = NULL;
	variable->flags = dynvVariable::Flags(0);
	return variable;
}

void dynv_variable_destroy_data(struct dynvVariable* variable){
	struct dynvVariable *next, *i;
	i = variable->next;

	while (i){
		next = i->next;
		if (i->handler->destroy!=NULL) i->handler->destroy(i);
		if (i->name) free(i->name);
		delete i;
		
		i = next;
	}
	
	variable->next = NULL;
	variable->value = NULL;
	variable->handler = NULL;
}

void dynv_variable_destroy(struct dynvVariable* variable){
	struct dynvVariable *next, *i;
	i = variable;

	while (i){
		next = i->next;
		if (i->handler->destroy!=NULL) i->handler->destroy(i);
		if (i->name) free(i->name);
		delete i;
		
		i = next;
	}
}
