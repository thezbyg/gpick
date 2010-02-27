/*
 * Copyright (c) 2009-2010, Albertas Vyšniauskas
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

#include "DynvMemoryIO.h"
#include <string.h>
#include <stdio.h>

struct dynvMemoryIO{
	char* buffer;
	uint32_t size;
	uint32_t eof;
	uint32_t position;
};

static int dynv_io_memory_write(struct dynvIO* io, void* data, uint32_t size, uint32_t* data_written) {
	struct dynvMemoryIO* mem_io = (struct dynvMemoryIO*) io->userdata;

	uint32_t data_left = mem_io->size - mem_io->position;
	if (data_left < size){ //buffer too small
		uint32_t new_buf = mem_io->size + size + 4096;

		//while (new_buf - mem_io->position < size)
		//	new_buf *= 2;

		char *nb;
		if ((nb = new char[new_buf])){
			if (mem_io->buffer){
				memcpy(nb, mem_io->buffer, mem_io->position);
				delete[] mem_io->buffer;
			}
			mem_io->buffer = nb;
			mem_io->size = new_buf;
		}else{
			*data_written = 0;
			return 0;
		}
	}
	memcpy(mem_io->buffer + mem_io->position, data, size);
	mem_io->position += size;
	if (mem_io->position > mem_io->eof)
		mem_io->eof = mem_io->position;
	*data_written = size;
	return 0;
}

static int dynv_io_memory_read(struct dynvIO* io, void* data, uint32_t size, uint32_t* data_read){
	struct dynvMemoryIO* mem_io = (struct dynvMemoryIO*) io->userdata;

	uint32_t data_left = mem_io->eof - mem_io->position;
	if (size > data_left)
		size = data_left;
	memcpy(data, mem_io->buffer + mem_io->position, size);
	mem_io->position += size;
	*data_read=size;
	return 0;
}

static int dynv_io_memory_seek(struct dynvIO* io, uint32_t offset, int type, uint32_t* position){
	struct dynvMemoryIO* mem_io = (struct dynvMemoryIO*) io->userdata;

	switch (type){
	case SEEK_CUR:
		mem_io->position+=offset;
		if (mem_io->position>mem_io->eof) mem_io->position=mem_io->eof;
		if(position) *position=mem_io->position;
		return 0;
		break;
	case SEEK_SET:
		mem_io->position=offset;
		if (mem_io->position>mem_io->eof) mem_io->position=mem_io->eof;
		if(position) *position=mem_io->position;
		return 0;
		break;
	case SEEK_END:
		mem_io->position=mem_io->eof-offset;
		if (mem_io->position>mem_io->eof) mem_io->position=mem_io->eof;
		if(position) *position=mem_io->position;
		return 0;
		break;
	}
	return -1;
}

static int dynv_io_memory_free(struct dynvIO* io){
	struct dynvMemoryIO* mem_io=(struct dynvMemoryIO*)io->userdata;
	if (mem_io->buffer) delete mem_io->buffer;
	delete mem_io;
	return 0;
}

static int dynv_io_memory_reset(struct dynvIO* io){
	struct dynvMemoryIO* mem_io=(struct dynvMemoryIO*)io->userdata;
	mem_io->eof=0;
	mem_io->position=0;
	return 0;
}

struct dynvIO* dynv_io_memory_new(){
	struct dynvIO* io=new struct dynvIO;
	struct dynvMemoryIO* mem_io=new struct dynvMemoryIO;

	mem_io->buffer=0;
	mem_io->eof=0;
	mem_io->position=0;
	mem_io->size=0;

	io->userdata=mem_io;

	io->write=dynv_io_memory_write;
	io->read=dynv_io_memory_read;
	io->seek=dynv_io_memory_seek;
	io->free=dynv_io_memory_free;
	io->reset=dynv_io_memory_reset;

	return io;
}

int dynv_io_memory_get_data(struct dynvIO* io, char** data, uint32_t* size){
	struct dynvMemoryIO* mem_io=(struct dynvMemoryIO*)io->userdata;
	if (!mem_io) return -1;
	if (!mem_io->buffer) return -1;
	*data=mem_io->buffer;
	*size=mem_io->eof;
	return 0;
}

int dynv_io_memory_set_data(struct dynvIO* io, char* data, uint32_t size){
	struct dynvMemoryIO* mem_io=(struct dynvMemoryIO*)io->userdata;
	if (!mem_io) return -1;
	dynv_io_memory_reset(io);

	uint32_t written;
	dynv_io_memory_write(io, data, size, &written);
	return 0;
}

int dynv_io_memory_prepare_size(struct dynvIO* io, uint32_t size){
	struct dynvMemoryIO* mem_io=(struct dynvMemoryIO*)io->userdata;
	if (!mem_io) return -1;

	mem_io->eof=size;
	mem_io->position=0;

	if (mem_io->size<size){

		char *nb;
		if ((nb = new char[size])){
			//memcpy(nb, mem_io->buffer, mem_io->position);
			if (mem_io->buffer) delete[] mem_io->buffer;
			mem_io->buffer = nb;
			mem_io->size = size;
			return 0;
		}else{
			return -1;
		}
	}
	return 0;
}

void* dynv_io_memory_get_buffer(struct dynvIO* io){
	struct dynvMemoryIO* mem_io=(struct dynvMemoryIO*)io->userdata;
	if (!mem_io) return 0;
	return mem_io->buffer;
}
