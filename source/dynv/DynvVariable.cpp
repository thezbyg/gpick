/*
 * Copyright (c) 2009-2016, Albertas Vyšniauskas
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

dynvVariable* dynv_variable_create(const char* name, dynvHandler* handler)
{
	struct dynvVariable* variable = new struct dynvVariable;
	if (name){
		variable->name = strdup(name);
	}else{
		variable->name = nullptr;
	}
	variable->handler = handler;
	variable->ptr_value = nullptr;
	variable->next = nullptr;
	variable->flags = dynvVariable::Flag::none;
	return variable;
}
void dynv_variable_destroy_data(dynvVariable* variable)
{
	dynvVariable *next, *i;
	i = variable->next;
	while (i){
		next = i->next;
		if (i->handler->destroy != nullptr) i->handler->destroy(i);
		if (i->name) free(i->name);
		delete i;
		i = next;
	}
	if (variable->handler->destroy != nullptr) variable->handler->destroy(variable);
	variable->next = nullptr;
	variable->ptr_value = nullptr;
	variable->handler = nullptr;
}
void dynv_variable_destroy(dynvVariable* variable)
{
	dynvVariable *next, *i;
	i = variable;
	while (i){
		next = i->next;
		if (i->handler->destroy != nullptr) i->handler->destroy(i);
		if (i->name) free(i->name);
		delete i;
		i = next;
	}
}
dynvVariable::Flag operator&(dynvVariable::Flag x, dynvVariable::Flag y)
{
	return static_cast<dynvVariable::Flag>(static_cast<uintptr_t>(x) & static_cast<uintptr_t>(y));
}
bool operator!=(dynvVariable::Flag x, dynvVariable::Flag y)
{
	return static_cast<uintptr_t>(x) != static_cast<uintptr_t>(y);
}
