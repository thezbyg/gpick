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

#include "FileFormat.h"
#include "Endian.h"
#include "dynv/DynvSystem.h"
#include "dynv/DynvMemoryIO.h"
#include <string.h>

#include <iostream>
#include <fstream>


using namespace std;

struct ChunkHeader{
	char type[16];
	unsigned int size;
};

#define CHUNK_TYPE_COLORLIST		"color_list"

int prepare_chunk_header(struct ChunkHeader* header, const char* type, unsigned long size){
	strncpy(header->type, type, 16);
	header->size=ULONG_TO_LE(size);
	return 0;
}

int palette_file_save(const char* filename, struct ColorList* color_list){
	ofstream file(filename, ios::binary);
	if (file.is_open()){
		struct dynvIO* mem_io=dynv_io_memory_new();
		char* data;
		unsigned long size;
		
		struct ChunkHeader header;
		ofstream::pos_type colorlist_pos = file.tellp();
		file.write((char*)&header, sizeof(header));

		struct dynvHandlerMap* handler_map = dynv_system_get_handler_map(color_list->params);
		dynv_handler_map_serialize(handler_map, mem_io);
		dynv_io_memory_get_data(mem_io, &data, &size);
		file.write(data, size);
		dynv_io_reset(mem_io);
		dynv_handler_map_release(handler_map);
		
		for (ColorList::iter i=color_list->colors.begin(); i!=color_list->colors.end(); ++i){			
			dynv_system_serialize((*i)->params, mem_io);
			dynv_io_memory_get_data(mem_io, &data, &size);
			file.write(data, size);			
			dynv_io_reset(mem_io);
		}
		
		dynv_io_free(mem_io);
		
		ofstream::pos_type end_pos = file.tellp();
		file.seekp(colorlist_pos);
		prepare_chunk_header(&header, CHUNK_TYPE_COLORLIST, end_pos-colorlist_pos);
		file.write((char*)&header, sizeof(header));
	
		file.close();
		return 0;
	}
	return -1;
}

