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

#include "DynvHandler.h"
#include "DynvVariable.h"
#include "DynvIO.h"

#include "../Endian.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <vector>
#include <iostream>
using namespace std;

bool dynvHandlerMap::dynvKeyCompare::operator() (const char* const& x, const char* const& y) const
{
	return strcmp(x,y)<0;
}


struct dynvHandler* dynv_handler_create(const char* name){
	struct dynvHandler* handler=new struct dynvHandler;
	handler->name=strdup(name);
	handler->create=NULL;
	handler->destroy=NULL;
	handler->set=NULL;
	handler->serialize=NULL;
	handler->deserialize=NULL;
	handler->serialize_xml=NULL;
	handler->deserialize_xml=NULL;
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

