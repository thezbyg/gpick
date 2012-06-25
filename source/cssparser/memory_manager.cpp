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


#include "memory_manager.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static struct MemoryPage* memory_page_new(uint32_t total_size, uint32_t atom_size){
	uint32_t atoms = total_size / atom_size;

	struct MemoryPage* page=(struct MemoryPage*)malloc(sizeof(struct MemoryPage) + atoms + total_size);
	page->total_size = atoms;
	page->total_free = atoms;
	page->atom_size = atom_size;

	page->map = ((uint8_t*)page) + sizeof(struct MemoryPage);
	page->raw_memory = ((uint8_t*)page) + sizeof(struct MemoryPage) + atoms;

	memset(page->map, 1, atoms);

	page->next_page=0;
	return page;
}

static void memory_page_destroy(struct MemoryPage* page){
	free(page);
}

static void* memory_page_alloc(struct MemoryPage* page, uint32_t alloc_size){
	register uint32_t size = (alloc_size + sizeof(uint32_t) + (page->atom_size-1))/page->atom_size;

	if (size>page->total_free) return 0;

	register uint32_t space=0;
	for (register uint32_t i=0; i<page->total_size; ++i){
		space=(space+1)*page->map[i];
		if (space==size){
			memset(page->map+(i+1-size), 0, size);
			page->total_free-=size;

			uint8_t *mem_ptr = page->raw_memory+(i+1-size)*page->atom_size;
			((uint32_t*)mem_ptr)[0] = alloc_size;
			return mem_ptr+sizeof(uint32_t);
		}
	}

	return 0;
}

bool memory_page_free(struct MemoryPage* page, void* mem_ptr){

	if ((mem_ptr >= page->raw_memory) && (mem_ptr < page->raw_memory + page->total_size * page->atom_size )){

		uint32_t atom_i = (uintptr_t(mem_ptr) - uintptr_t(page->raw_memory) - sizeof(uint32_t)) / page->atom_size;
		uint32_t atom_count = ((((uint32_t*)mem_ptr)-1)[0] + sizeof(uint32_t) + (page->atom_size-1))/page->atom_size;

		memset(page->map+atom_i, 1, atom_count);
		page->total_free+=atom_count;

		return true;
	}
	return false;
}

static uint32_t high_bit(uint32_t x){
	register uint32_t bit=32;
	register uint32_t mask=(1<<(bit-1));

	while (!(x & mask)){
		mask>>=1;
		--bit;
		if (!mask) break;
	}
	return bit;
}

void* memory_alloc(struct Memory* memory, uint32_t size){

	struct MemoryPage* page=memory->pages;
	void* ptr;

	struct {
		int32_t atom_size;
		int32_t memory_size;
	}segments[]={
		{1<<5, (1<<5)*4096},		// 0
		{1<<5, (1<<5)*4096},		// 1
		{1<<5, (1<<5)*4096},		// 2
		{1<<5, (1<<5)*4096},		// 3
		{1<<5, (1<<5)*4096},		// 4
		{1<<5, (1<<5)*4096},		// 5
		{1<<5, (1<<5)*4096},		// 6
		{1<<5, (1<<5)*4096},		// 7
		{1<<5, (1<<5)*4096},		// 8

		{1<<7, (1<<7)*1024},		// 9
		{1<<7, (1<<7)*1024},		// 10
		{1<<7, (1<<7)*1024},		// 11
		{1<<7, (1<<7)*1024},		// 12

		{1<<8, (1<<8)*512},			// 13
		{1<<8, (1<<8)*512},			// 14
		{1<<8, (1<<8)*512},			// 15

		{1<<12, (1<<12)*256},		// 16
		{1<<12, (1<<12)*256},		// 17
		{1<<12, (1<<12)*256},		// 18
		{1<<12, (1<<12)*256},		// 19
									// 1MB and more
	};

	uint32_t highbit = high_bit(size);

	if (page){
		do{
			if (page->atom_size != segments[highbit].atom_size) continue;
			if ((ptr = memory_page_alloc(page, size))) return ptr;
		}while ((page = page->next_page));
	}

	//printf("new page: %d %d %d %d\n", segments[highbit].memory_size, segments[highbit].atom_size, highbit, size);

	page = memory_page_new( segments[highbit].memory_size, segments[highbit].atom_size);
	page->next_page = memory->pages;
	memory->pages = page;

	if ((ptr = memory_page_alloc(page, size))) return ptr;

	exit(0);
	return 0;
}

void* memory_realloc(struct Memory* memory, void* mem_ptr, uint32_t size){

	//printf("r: %d\n", size);

	void* newptr=memory_alloc(memory, size);
	memcpy(newptr, mem_ptr, (((uint32_t*)mem_ptr)-1)[0]);
	memory_free(memory, mem_ptr);
	return newptr;
}

void memory_free(struct Memory* memory, void* mem_ptr){

	struct MemoryPage* page=memory->pages;

	if (page){
		do{
			if (memory_page_free(page, mem_ptr)) return;
		}while ((page = page->next_page));
	}
}



struct Memory* memory_new(){
	struct Memory* memory=(struct Memory*)malloc(sizeof(struct Memory));
	memory->pages=0;
	return memory;
}

void memory_destroy(struct Memory* memory){

	struct MemoryPage* page=memory->pages;
	struct MemoryPage* tmp;

	while (page){
		tmp=page;
		page=page->next_page;
		memory_page_destroy(tmp);
	}

	free(memory);

}

