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

#include "Random.h"

#include <string.h>

static unsigned long random_function_znew(struct Random* r, unsigned long seed_offset) {
	return (r->seed[seed_offset] = 36969 * (r->seed[seed_offset] & 65535) + (r->seed[seed_offset] >> 16));
}
static unsigned long random_function_wnew(struct Random* r, unsigned long seed_offset) {
	return (r->seed[seed_offset] = 18000 * (r->seed[seed_offset] & 65535) + (r->seed[seed_offset] >> 16));
}

static unsigned long random_function_MWC(struct Random* r, unsigned long seed_offset) {
	return (random_function_znew(r, 0)<<16) + random_function_wnew(r, 1);
}

static unsigned long random_function_SHR3(struct Random* r, unsigned long seed_offset) {
	r->seed[seed_offset] ^= r->seed[seed_offset] << 17;
	r->seed[seed_offset] ^= (r->seed[seed_offset]&0xFFFFFFFF) >> 13;
	return (r->seed[seed_offset] ^= r->seed[seed_offset] << 5);
}



struct Random* random_new(const char* random_function){
	struct Random* r=new struct Random;

	struct RandomFunction{
		const char* name;
		unsigned long (*function)(struct Random* r, unsigned long seed_offset);
		unsigned long seed_size;
	};

	struct RandomFunction functions[]={
		{"znew",	random_function_znew,	1},
		{"wnew",	random_function_wnew,	1},
		{"MWC",		random_function_MWC,	2},
		{"SHR3",	random_function_SHR3,	1},
	};

	int found=0;

	for (unsigned long i=0; i<sizeof(functions)/sizeof(struct RandomFunction); ++i){
		if (strcmp(random_function, functions[i].name)==0){
			r->function= functions[i].function;
			r->seed_size=functions[i].seed_size;
			r->seed=new unsigned long [r->seed_size];
			found=1;
			break;
		}
	}

	if (found) return r;
	delete r;
	return 0;
}

unsigned long random_get(struct Random* r){
	if (!r) return 0;
	return r->function(r, 0);
}

void random_seed(struct Random* r, void* seed){
	if (!r) return;
	memcpy(r->seed, seed, r->seed_size*sizeof(unsigned long));
}

void random_destroy(struct Random* r){
	if (r){
		delete [] r->seed;
		delete r;
	}
}
