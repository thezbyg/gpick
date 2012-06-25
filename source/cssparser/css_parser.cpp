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

#include <stdio.h>

#include "css_parser.h"
#include "css_lex.h"

#include <sstream>

using namespace std;

namespace css_parser{

css_base::css_base(){

}

void css_base::polymorphic(){

}

void* css_base::operator new(size_t num_bytes, parse_parm* parm){
	return memory_alloc(parm->memory, num_bytes);
}

void css_base::operator delete(void* base, parse_parm* parm){
	memory_free(parm->memory, base);
}




css_property::css_property(const char* name){
	name_ = name;
}

void css_property::addValue(css_base* value){
	values_.push_back(value);
}

css_simple_selector::css_simple_selector(const char* name){
	name_ = name;
}

css_selector::css_selector(){
}

void css_selector::addSimpleSelector(css_simple_selector* simple_selector){
  simple_selectors_.push_back(simple_selector);
}
void css_selector::addSelector(css_selector* selector){
	for (list<css_simple_selector*>::iterator i = selector->simple_selectors_.begin(); i != selector->simple_selectors_.end(); i++){
		simple_selectors_.push_back(*i);
	}
}

void css_selector::prependSimpleSelector(css_simple_selector* simple_selector){
  simple_selectors_.push_front(simple_selector);
}

css_selectors::css_selectors(){

}

void css_selectors::addSelector(css_selector* selector){
  selectors_.push_back(selector);
}


css_ruleset::css_ruleset(){

}

void css_ruleset::addSelector(css_selector* selector){
  selectors_.push_back(selector);
}

void css_ruleset::setSelectors(std::list<css_selector*> &selectors){
	selectors_ = selectors;
}

void css_ruleset::addProperty(css_property* property){
  properties_.push_back(property);
}

void css_ruleset::addProperties(css_properties* properties){
	for (list<css_property*>::iterator i = properties->properties_.begin(); i != properties->properties_.end(); i++){
		properties_.push_back(*i);
	}
}


css_file::css_file(){

}

void css_file::addRuleset(css_ruleset* ruleset){
  rulesets_.push_back(ruleset);
}



css_function::css_function(const char* name){
	name_ = name;
}

void css_function::addArgument(css_base* argument){
  arguments_.push_back(argument);
}


css_hex::css_hex(uint32_t value){
	value_ = value;
}

css_hex::css_hex(const char *value){
	uint32_t x;
  std::stringstream ss;
  ss << (value + 1);
  ss << hex;
	ss >> x;

	value_ = x;
	if (strlen(value) == 4){
		value_ = (value_ & 0xf00) << 12 | (value_ & 0xf00) << 8 | (value_ & 0x0f0) << 8 | (value_ & 0x0f0) << 4 | (value_ & 0x00f) << 4 | (value_ & 0x00f) << 0;
	}
}

css_number::css_number(double value){
	value_ = value;
}

css_percentage::css_percentage(double value){
	value_ = value;
}

css_string::css_string(const char* value){
	value_ = value;
}


css_properties::css_properties(){

}

void css_properties::addProperty(css_property* property){
  properties_.push_back(property);
}

}

using namespace css_parser;

void *css_parseAlloc(void *(*mallocProc)(size_t));
void css_parseFree(void *p, void (*freeProc)(void*));
void css_parse(void *yyp, int yymajor, css_yystype yyminor, parse_parm*);

void print_rec(css_base *node){

	css_file *file;
	css_ruleset *ruleset;
	css_selector *selector;
	css_property *property;
	css_simple_selector *simple_selector;

	if ((file = dynamic_cast<css_file*>(node))){
		printf("file\n");
		for (list<css_ruleset*>::iterator i = file->rulesets_.begin(); i != file->rulesets_.end(); i++){
			print_rec(*i);
		}
	}else	if ((ruleset = dynamic_cast<css_ruleset*>(node))){

		if (ruleset->properties_.begin() == ruleset->properties_.end()) return;

		printf("ruleset\n");

		for (list<css_selector*>::iterator i = ruleset->selectors_.begin(); i != ruleset->selectors_.end(); i++){
			print_rec(*i);
		}
		for (list<css_property*>::iterator i = ruleset->properties_.begin(); i != ruleset->properties_.end(); i++){
			print_rec(*i);
		}
		printf("\n");

	}else	if ((selector = dynamic_cast<css_selector*>(node))){
		for (list<css_simple_selector*>::iterator i = selector->simple_selectors_.begin(); i != selector->simple_selectors_.end(); i++){
			print_rec(*i);
		}
		printf("-> ");
	}else	if ((simple_selector = dynamic_cast<css_simple_selector*>(node))){
		printf("%s ", simple_selector->name_);
	}else	if ((property = dynamic_cast<css_property*>(node))){
		printf("%s = %06x, ", property->name_, ((css_hex*)*property->values_.begin())->value_);

	}

}

int parse_file(const char *filename){

	FILE* f = fopen(filename, "rt");

	yyscan_t scanner;
	css_yystype stype;
	parse_parm  pp;

	struct Memory* memory = memory_new();
	pp.memory = memory;
	pp.page = new(&pp) css_file;

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

	print_rec(pp.page);

	memory_destroy(memory);

	fclose(f);

	printf("\n");

	return 1;
}

