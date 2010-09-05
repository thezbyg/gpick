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

#include <stdio.h>

#include "css_parser.h"
#include "css_lex.h"

void* css_base::operator new(size_t num_bytes, parse_parm* parm){
	return memory_alloc(parm->memory, num_bytes);
}

void css_base::operator delete(void* base, parse_parm* parm){
	memory_free(parm->memory, base);
}

css_base* css_parser_push_base(css_base* base, css_base* x){

	if (!base) return x;
	if (x){
		base->end->next = x;
		base->end = x->end;
	}
	return base;
}



void *css_parseAlloc(void *(*mallocProc)(size_t));
void css_parseFree(void *p, void (*freeProc)(void*));
void css_parse(void *yyp, int yymajor, css_yystype yyminor, parse_parm*);


int parse_file(const char *filename){

	FILE* f = fopen(filename, "rt");

	yyscan_t scanner;
	css_yystype stype;
	parse_parm  pp;

	struct Memory* memory = memory_new();
	pp.memory = memory;
	pp.page = new(&pp) css_base;

	css_lex_init_extra(&pp, &scanner);
	css_set_in(f, scanner);
	css_restart(f, scanner);

	void *parser = css_parseAlloc(malloc);

	int token_id;
	while( (token_id = css_lex(&stype, scanner)) != 0){
		css_parse(parser, token_id, stype, &pp);
	}

	css_parseFree(parser, free);

	css_lex_destroy(scanner);
	memory_destroy(memory);

	fclose(f);

	printf("\n");

	return 1;
}

