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
#include <list>
#include <algorithm>

using namespace std;

struct ChunkHeader{
	char type[16];
	uint64_t size;
};

#define CHUNK_TYPE_HANDLER_MAP			"handler_map"
#define CHUNK_TYPE_COLOR_LIST			"color_list"
#define CHUNK_TYPE_COLOR_POSITIONS		"color_positions"

static int prepare_chunk_header(struct ChunkHeader* header, const char* type, uint64_t size){
	size_t len=strlen(type);
	if (len>=sizeof(header->type)) len=sizeof(header->type)-1;
	memcpy(header->type, type, len);
	memset(header->type+len, 0, sizeof(header->type)-len);

	//strncpy(header->type, type, 16);
	header->size=UINT64_TO_LE(size);
	return 0;
}

static int check_chunk_header(struct ChunkHeader* header){
	if (header->type[sizeof(header->type)-1]!=0) return -1;
	return 0;
}

static bool color_object_position_sort(struct ColorObject* x, struct ColorObject* y){
	return x->position < y->position;
}

int palette_file_load(const char* filename, struct ColorList* color_list) {
	ifstream file(filename, ios::binary);
	if (file.is_open()){
		struct dynvIO* mem_io = dynv_io_memory_new();

		struct ChunkHeader header;
		file.read((char*) &header, sizeof(header));
		if (file.fail()){
			file.close();
			return -1;
		}

		struct dynvHandlerMap* handler_map = dynv_system_get_handler_map(color_list->params);
		dynvHandlerMap::HandlerVec handler_vec;

		list<struct ColorObject*> color_objects;

		while (check_chunk_header(&header) == 0){

			//cout<<header.type<<" "<<header.size<<endl;


			if (strncmp(CHUNK_TYPE_HANDLER_MAP, header.type, sizeof(header.type)) == 0){
				dynv_io_memory_prepare_size(mem_io, header.size);
				file.read((char*) dynv_io_memory_get_buffer(mem_io), header.size);
				handler_vec.clear();

				dynv_handler_map_deserialize(handler_map, mem_io, handler_vec);
			}else if (strncmp(CHUNK_TYPE_COLOR_LIST, header.type, sizeof(header.type)) == 0){
				dynv_io_memory_prepare_size(mem_io, header.size);
				file.read((char*) dynv_io_memory_get_buffer(mem_io), header.size);

				for (;;){
					struct ColorObject* color = color_object_new(handler_map);
					if (dynv_system_deserialize(color->params, handler_vec, mem_io)==0){
						//color_list_add_color_object(color_list, color, 0);
						//color_object_release(color);
						color_objects.push_back(color);
					}else{
						break;
					}
				}

			}else if (strncmp(CHUNK_TYPE_COLOR_POSITIONS, header.type, sizeof(header.type)) == 0){
				dynv_io_memory_prepare_size(mem_io, header.size);
				file.read((char*) dynv_io_memory_get_buffer(mem_io), header.size);

				uint32_t index, read;
				for (list<struct ColorObject*>::iterator i=color_objects.begin(); i!=color_objects.end(); ++i){
					if (dynv_io_read(mem_io, &index, sizeof(uint32_t), &read)==0){
						if (read!=sizeof(uint32_t)) break;
						(*i)->position=index;
					}
				}

				color_objects.sort(color_object_position_sort);

				for (list<struct ColorObject*>::iterator i=color_objects.begin(); i!=color_objects.end(); ++i){
					color_list_add_color_object(color_list, *i, ((*i)->position!=~(uint32_t)0));
					color_object_release(*i);
				}



			}else{
				file.seekg(header.size, ios_base::cur);
				if (file.fail() || file.eof())
					break;
			}

			file.read((char*) &header, sizeof(header));
			if (file.fail())
				break;
		}

		dynv_handler_map_release(handler_map);

		file.close();
		return 0;
	}
	return -1;
}

int palette_file_save(const char* filename, struct ColorList* color_list){
	ofstream file(filename, ios::binary);
	if (file.is_open()){
		struct dynvIO* mem_io=dynv_io_memory_new();
		char* data;
		uint32_t size;
		
		ofstream::pos_type end_pos;

		struct ChunkHeader header;


		ofstream::pos_type handler_map_pos = file.tellp();
		file.write((char*)&header, sizeof(header));

		struct dynvHandlerMap* handler_map = dynv_system_get_handler_map(color_list->params);
		dynv_handler_map_serialize(handler_map, mem_io);
		dynv_io_memory_get_data(mem_io, &data, &size);
		file.write(data, size);
		dynv_io_reset(mem_io);
		dynv_handler_map_release(handler_map);
		
		end_pos = file.tellp();
		file.seekp(handler_map_pos);
		prepare_chunk_header(&header, CHUNK_TYPE_HANDLER_MAP, end_pos-handler_map_pos-sizeof(struct ChunkHeader));
		file.write((char*)&header, sizeof(header));
		file.seekp(end_pos);


		ofstream::pos_type colorlist_pos = file.tellp();
		file.write((char*)&header, sizeof(header));

		/*struct dynvHandlerMap* handler_map = dynv_system_get_handler_map(color_list->params);
		dynv_handler_map_serialize(handler_map, mem_io);
		dynv_io_memory_get_data(mem_io, &data, &size);
		file.write(data, size);
		dynv_io_reset(mem_io);
		dynv_handler_map_release(handler_map);*/

		for (ColorList::iter i=color_list->colors.begin(); i!=color_list->colors.end(); ++i){			
			dynv_system_serialize((*i)->params, mem_io);
			dynv_io_memory_get_data(mem_io, &data, &size);
			file.write(data, size);			
			dynv_io_reset(mem_io);
		}
		
		dynv_io_free(mem_io);
		
		end_pos = file.tellp();
		file.seekp(colorlist_pos);
		prepare_chunk_header(&header, CHUNK_TYPE_COLOR_LIST, end_pos-colorlist_pos-sizeof(struct ChunkHeader));
		file.write((char*)&header, sizeof(header));
		file.seekp(end_pos);


		color_list_get_positions(color_list);

		uint32_t *positions=new uint32_t [color_list->colors.size()];
		uint32_t *position=positions;
		for (ColorList::iter i=color_list->colors.begin(); i!=color_list->colors.end(); ++i){
			*position=UINT32_TO_LE((*i)->position);

			++position;
		}

		prepare_chunk_header(&header, CHUNK_TYPE_COLOR_POSITIONS, color_list->colors.size()*sizeof(uint32_t));
		file.write((char*)&header, sizeof(header));
		file.write((char*)positions, color_list->colors.size()*sizeof(long));

		delete [] positions;
	
		file.close();
		return 0;
	}
	return -1;
}

