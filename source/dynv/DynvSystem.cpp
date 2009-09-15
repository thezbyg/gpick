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
#include "../Endian.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <vector>
#include <iostream>
using namespace std;

bool dynvKeyCompare::operator() (const char* const& x, const char* const& y) const
{
	return strcmp(x,y)<0;
}

struct dynvVariable* dynv_variable_create(const char* name, struct dynvHandler* handler){
	struct dynvVariable* variable=new struct dynvVariable;
	variable->name=strdup(name);
	variable->handler=handler;
	variable->value=NULL;
	return variable;
}

void dynv_variable_destroy(struct dynvVariable* variable){
	if (variable->handler->destroy!=NULL) variable->handler->destroy(variable);
	free(variable->name);
	delete variable;
}

void dynv_variable_set_flags(struct dynvVariable* variable, long flags){
	variable->flags|=flags;
}


struct dynvHandler* dynv_handler_create(const char* name){
	struct dynvHandler* handler=new struct dynvHandler;
	handler->name=strdup(name);
	handler->create=NULL;
	handler->destroy=NULL;
	handler->set=NULL;
	handler->serialize=NULL;
	handler->deserialize=NULL;
	handler->get=NULL;
	return handler;
}

void dynv_handler_destroy(struct dynvHandler* handler){
	free(handler->name);
	delete handler;
}



struct dynvHandlerMap* dynv_handler_map_create(){
	struct dynvHandlerMap* handler_map=new struct dynvHandlerMap;
	handler_map->refcnt=0;
	return handler_map;
}

int dynv_handler_map_release(struct dynvHandlerMap* handler_map){
	if (handler_map->refcnt){
		handler_map->refcnt--;
		return -1;
	}else{
		dynvHandlerMap::HandlerMap::iterator i;

		for (i=handler_map->handlers.begin(); i!=handler_map->handlers.end(); ++i){
			dynv_handler_destroy((*i).second);
		}
		handler_map->handlers.clear();

		delete handler_map;
		return 0;
	}
}

struct dynvHandlerMap* dynv_handler_map_ref(struct dynvHandlerMap* handler_map){
	handler_map->refcnt++;
	return handler_map;
}

int dynv_handler_map_add_handler(struct dynvHandlerMap* handler_map, struct dynvHandler* handler){
	dynvHandlerMap::HandlerMap::iterator i;

	i=handler_map->handlers.find(handler->name);
	if (i!=handler_map->handlers.end()){
		return -1;
	}else{
		handler_map->handlers[handler->name]=handler;
		return 0;
	}

}

struct dynvHandler* dynv_handler_map_get_handler(struct dynvHandlerMap* handler_map, const char* handler_name){
	dynvHandlerMap::HandlerMap::iterator i;

	i=handler_map->handlers.find(handler_name);
	if (i!=handler_map->handlers.end()){
		return (*i).second;
	}else{
		return 0;
	}
}

int dynv_handler_map_serialize(struct dynvHandlerMap* handler_map, struct dynvIO* io){
	dynvHandlerMap::HandlerMap::iterator i;
	uint32_t written, length;
	uint32_t id=0;

	uint32_t handler_count=handler_map->handlers.size();
	handler_count=UINT32_TO_LE(handler_count);
	dynv_io_write(io, &handler_count, 4, &written);

	for (i=handler_map->handlers.begin(); i!=handler_map->handlers.end(); ++i){
		struct dynvHandler* handler=(*i).second;

		length=strlen(handler->name);
		uint32_t length_le=UINT32_TO_LE(length);

		dynv_io_write(io, &length_le, 4, &written);
		dynv_io_write(io, handler->name, length, &written);

		handler->id=id;
		id++;
	}
	return 0;
}

int dynv_handler_map_deserialize(struct dynvHandlerMap* handler_map, struct dynvIO* io, dynvHandlerMap::HandlerVec& handler_vec){
	uint32_t read;
	uint32_t handler_count;
	uint32_t length;
	char* name;
	struct dynvHandler* handler;

	if (dynv_io_read(io, &handler_count, 4, &read)==0){
		if (read!=4) return -1;
	}else return -1;

	handler_count=UINT32_TO_LE(handler_count);

	handler_vec.resize(handler_count);

	for (uint32_t i=0; i!=handler_count; ++i){
		dynv_io_read(io, &length, 4, &read);
		length=UINT32_TO_LE(length);
		name=new char [length+1];
		dynv_io_read(io, name, length, &read);
		name[length]=0;

		handler=dynv_handler_map_get_handler(handler_map, name);
		handler_vec[i]=handler;

		//cout<<"Handler: "<< name<<endl;

		delete [] name;
	}

	return 0;
}


struct dynvHandlerMap* dynv_system_get_handler_map(struct dynvSystem* dynv_system){
	return dynv_handler_map_ref(dynv_system->handler_map);
}

void dynv_system_set_handler_map(struct dynvSystem* dynv_system, struct dynvHandlerMap* handler_map){
	if (dynv_system->handler_map!=NULL){
		dynv_handler_map_release(dynv_system->handler_map);
		dynv_system->handler_map=NULL;
	}
	if (handler_map!=NULL){
		dynv_system->handler_map=dynv_handler_map_ref(handler_map);
	}
}

struct dynvSystem* dynv_system_create(struct dynvHandlerMap* handler_map){
	struct dynvSystem* dynv_system=new struct dynvSystem;
	dynv_system->handler_map=NULL;
	dynv_system->refcnt=0;
	dynv_system_set_handler_map(dynv_system, handler_map);
	return dynv_system;
}

int dynv_system_release(struct dynvSystem* dynv_system){
	if (dynv_system->refcnt){
		dynv_system->refcnt--;
		return -1;
	}else{
		dynvSystem::VariableMap::iterator i;

		for (i=dynv_system->variables.begin(); i!=dynv_system->variables.end(); ++i){
			dynv_variable_destroy((*i).second);
		}
		dynv_system->variables.clear();

		dynv_handler_map_release(dynv_system->handler_map);

		delete dynv_system;
		return 0;
	}
}

struct dynvSystem* dynv_system_ref(struct dynvSystem* dynv_system){
	dynv_system->refcnt++;
	return dynv_system;
}

struct dynvVariable* dynv_system_add_empty(struct dynvSystem* dynv_system, struct dynvHandler* handler, const char* variable_name){
	struct dynvVariable* variable=NULL;

	dynvSystem::VariableMap::iterator i;
	i=dynv_system->variables.find(variable_name);
	if (i==dynv_system->variables.end()){
		if (handler==NULL) return 0;
		variable=dynv_variable_create(variable_name, handler);
		dynv_system->variables[variable->name]=variable;
		variable->handler->create(variable);
		return variable;
	}else{
		variable=(*i).second;
	}

	if (variable->handler==handler){
		return variable;
	}else{
		if (handler->create!=NULL){
			variable->handler->destroy(variable);
			variable->handler=handler;
			variable->handler->create(variable);
			return variable;
		}
	}
	return 0;
}

int dynv_system_set(struct dynvSystem* dynv_system, const char* handler_name, const char* variable_name, void* value){
	struct dynvVariable* variable=NULL;
	struct dynvHandler* handler=NULL;

	if (handler_name!=NULL){
		dynvHandlerMap::HandlerMap::iterator j;
		j=dynv_system->handler_map->handlers.find(handler_name);
		if (j==dynv_system->handler_map->handlers.end()){
			return -3;
		}else{
			handler=(*j).second;
		}
	}

	dynvSystem::VariableMap::iterator i;
	i=dynv_system->variables.find(variable_name);
	if (i==dynv_system->variables.end()){
		if (handler==NULL) return -2;
		variable=dynv_variable_create(variable_name, handler);
		dynv_system->variables[variable->name]=variable;
		variable->handler->create(variable);
		return variable->handler->set(variable, value);
	}else{
		variable=(*i).second;
	}

	if (variable->handler==handler){
		return variable->handler->set(variable, value);
	}else{
		if (handler->create!=NULL){
			variable->handler->destroy(variable);
			variable->handler=handler;
			variable->handler->create(variable);
			return variable->handler->set(variable, value);
		}
	}
	return -1;
}

void* dynv_system_get(struct dynvSystem* dynv_system, const char* handler_name, const char* variable_name){
	struct dynvVariable* variable=NULL;
	struct dynvHandler* handler=NULL;

	if (handler_name!=NULL){
		dynvHandlerMap::HandlerMap::iterator j;
		j=dynv_system->handler_map->handlers.find(handler_name);
		if (j==dynv_system->handler_map->handlers.end()){
			return 0;
		}else{
			handler=(*j).second;
		}
	}

	dynvSystem::VariableMap::iterator i;
	i=dynv_system->variables.find(variable_name);
	if (i==dynv_system->variables.end()){
		return 0;
	}else{
		variable=(*i).second;
	}

	if (variable->handler==handler){
		if (variable->handler->get!=NULL){
			void* value;
			if (variable->handler->get(variable, &value)==0){
				return value;
			}else{
				return 0;
			}
		}
	}
	return 0;
}

int dynv_system_remove(struct dynvSystem* dynv_system, const char* variable_name){
	struct dynvVariable* variable=NULL;

	dynvSystem::VariableMap::iterator i;
	i=dynv_system->variables.find(variable_name);
	if (i==dynv_system->variables.end()){
		return -1;
	}else{
		variable=(*i).second;
		variable->handler->destroy(variable);
		dynv_system->variables.erase(i);
		return 0;
	}
}

int dynv_system_remove_all(struct dynvSystem* dynv_system){
	dynvSystem::VariableMap::iterator i;

	for (i=dynv_system->variables.begin(); i!=dynv_system->variables.end(); ++i){
		dynv_variable_destroy((*i).second);
	}
	dynv_system->variables.clear();
	return 0;
}

struct dynvVariable* dynv_system_get_var(struct dynvSystem* dynv_system, const char* variable_name){

	dynvSystem::VariableMap::iterator i;
	i=dynv_system->variables.find(variable_name);
	if (i==dynv_system->variables.end()){
		return 0;
	}else{
		return (*i).second;
	}
}


int dynv_system_serialize(struct dynvSystem* dynv_system, struct dynvIO* io){

	dynvSystem::VariableMap::iterator i;
	uint32_t written, length, id;

	uint32_t variable_count=dynv_system->variables.size();
	variable_count=UINT32_TO_LE(variable_count);
	dynv_io_write(io, &variable_count, 4, &written);

	uint32_t handler_count=dynv_system->handler_map->handlers.size();

	int_fast32_t handler_bytes;
	if (handler_count<=0xFF) handler_bytes=1;
	else if (handler_count<=0xFFFF) handler_bytes=2;
	else if (handler_count<=0xFFFFFF) handler_bytes=3;
	else handler_bytes=4;

	for (i=dynv_system->variables.begin(); i!=dynv_system->variables.end(); ++i){
		struct dynvVariable* variable=(*i).second;

		id=UINT32_TO_LE(variable->handler->id);
		dynv_io_write(io, &id, handler_bytes, &written);

		length=strlen(variable->name);
		uint32_t length_le=UINT32_TO_LE(length);

		dynv_io_write(io, &length_le, 4, &written);
		dynv_io_write(io, variable->name, length, &written);

		variable->handler->serialize(variable, io);

	}
	return 0;
}

int dynv_system_deserialize(struct dynvSystem* dynv_system, dynvHandlerMap::HandlerVec& handler_vec, struct dynvIO* io){

	uint32_t read;
	uint32_t variable_count, handler_id;
	uint32_t length=0;
	char* name;
	struct dynvVariable* variable;

	if (dynv_io_read(io, &variable_count, 4, &read)==0){
		if (read!=4) return -1;
	}else return -1;

	variable_count=UINT32_FROM_LE(variable_count);

	int_fast32_t handler_bytes;
	if (handler_vec.size()<=0xFF) handler_bytes=1;
	else if (handler_vec.size()<=0xFFFF) handler_bytes=2;
	else if (handler_vec.size()<=0xFFFFFF) handler_bytes=3;
	else handler_bytes=4;

	for (uint32_t i=0; i!=variable_count; ++i){
		handler_id=0;
		dynv_io_read(io, &handler_id, handler_bytes, &read);
		handler_id=UINT32_FROM_LE(handler_id);

		if ((handler_id<handler_vec.size()) && (handler_vec[handler_id])){

			dynv_io_read(io, &length, 4, &read);
			length=UINT32_FROM_LE(length);
			name=new char [length+1];
			dynv_io_read(io, name, length, &read);
			name[length]=0;

			variable=dynv_system_add_empty(dynv_system, handler_vec[handler_id], name);
			if (variable){
				//cout<<"Var: "<< name<<" "<<handler_id<<" "<<handler_vec[handler_id]->name<<endl;
				if (handler_vec[handler_id]->deserialize(variable, io)!=0){
					dynv_io_read(io, &length, 4, &read);
					length=UINT32_FROM_LE(length);
					dynv_io_seek(io, length, SEEK_CUR, 0);
				}
			}else{
				//cout<<"Var: skipping val"<<endl;
				dynv_io_read(io, &length, 4, &read);
				length=UINT32_FROM_LE(length);
				dynv_io_seek(io, length, SEEK_CUR, 0);
			}
			delete [] name;

		}else{

			dynv_io_read(io, &length, 4, &read);
			length=UINT32_FROM_LE(length);
			dynv_io_seek(io, length, SEEK_CUR, 0);

			dynv_io_read(io, &length, 4, &read);
			length=UINT32_FROM_LE(length);
			dynv_io_seek(io, length, SEEK_CUR, 0);
		}
	}
	return 0;
}


int dynv_io_write(struct dynvIO* io, void* data, uint32_t size, uint32_t* data_written) {
	return io->write(io, data, size, data_written);
}

int dynv_io_read(struct dynvIO* io, void* data, uint32_t size, uint32_t* data_read) {
	return io->read(io, data, size, data_read);
}

int dynv_io_seek(struct dynvIO* io, uint32_t offset, int type, uint32_t* position) {
	return io->seek(io, offset, type, position);
}

int dynv_io_free(struct dynvIO* io) {
	int r = io->free(io);
	delete io;
	return r;
}
int dynv_io_reset(struct dynvIO* io) {
	if (io->reset) return io->reset(io);
	return -1;
}

struct dynvSystem* dynv_system_copy(struct dynvSystem* dynv_system){
	
	struct dynvHandlerMap* handler_map = dynv_system_get_handler_map(dynv_system);
	struct dynvSystem* new_dynv = dynv_system_create(handler_map);
	dynv_handler_map_release(handler_map);
	
	void* value;
	struct dynvVariable *variable, *new_variable;
	struct dynvHandler* handler;
	
	dynvSystem::VariableMap::iterator i;
	for (i=dynv_system->variables.begin(); i!=dynv_system->variables.end(); ++i){
		
		variable = (*i).second;
		handler = (*i).second->handler;
		
		if (handler->get(variable, &value)==0){
			new_variable = dynv_variable_create(variable->name, handler);
			new_dynv->variables[new_variable->name] = new_variable;
			new_variable->handler->create(new_variable);
			new_variable->handler->set(new_variable, value);
		}
	}
	
	return new_dynv;
}

