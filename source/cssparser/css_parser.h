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

#ifndef CSS_PARSER_H
#define CSS_PARSER_H

#ifndef _MSC_VER
#include <stdbool.h>
#endif
#include <stdint.h>
#include <stdio.h>

#include <list>

#include "memory_manager.h"

typedef struct parse_parm_s parse_parm;

namespace css_parser {
class css_file;
class css_base;
}

typedef struct parse_parm_s{
	css_parser::css_file *page;
	struct Memory* memory;

	uint32_t first_line;
	uint32_t first_column;
	uint32_t last_line;
	uint32_t last_column;
	uint32_t position;
}parse_parm;

int parse(FILE* f, int *result);

#define YY_EXTRA_TYPE   parse_parm*

#ifndef YYSTYPE
typedef union css_yystype{
	css_parser::css_base *base;
	char* string;
	uint32_t number;
} css_yystype;
# define YYSTYPE css_yystype
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif

/* extern YYSTYPE yylval; */
//YYSTYPE yylval;

/*#define YY_EXTRA_TYPE   parse_parm* */

/* Initialize LOC. */
# define CSS_LOCATION_RESET(Loc)                  	\
	(Loc).first_column = (Loc).first_line = 1;  		\
	(Loc).last_column =  (Loc).last_line = 1;			\
	(Loc).position=0;

/* Advance of NUM lines. */
# define CSS_LOCATION_LINES(Loc, Num)             \
  (Loc).last_column = 1;                      \
  (Loc).last_line += Num;

/* Restart: move the first cursor to the last position. */
# define CSS_LOCATION_STEP(Loc)                   \
  (Loc).first_column = (Loc).last_column;     \
  (Loc).first_line = (Loc).last_line;

/* Output LOC on the stream OUT. */
# define CSS_LOCATION_PRINT(Out, Loc)                               \
  if ((Loc).first_line != (Loc).last_line)                      \
    fprintf (Out, "%d.%d-%d.%d",                                \
             (Loc).first_line, (Loc).first_column,              \
             (Loc).last_line, (Loc).last_column - 1);           \
  else if ((Loc).first_column < (Loc).last_column - 1)          \
    fprintf (Out, "%d.%d-%d", (Loc).first_line,                 \
             (Loc).first_column, (Loc).last_column - 1);        \
  else                                                          \
    fprintf (Out, "%d.%d", (Loc).first_line, (Loc).first_column)


namespace css_parser {

class css_base{
public:
	css_base();

	void* operator new(size_t num_bytes, parse_parm* parm);
	void operator delete(void* base, parse_parm* parm);

	virtual void polymorphic();
};

class css_property: public css_base{
public:
	const char* name_;
	std::list<css_base*> values_;

	css_property(const char* name);

	void addValue(css_base* value);
};

class css_properties: public css_base{
public:
	std::list<css_property*> properties_;

	css_properties();

	void addProperty(css_property* property);
};

class css_simple_selector: public css_base{
public:
	const char* name_;

	css_simple_selector(const char* name);
};

class css_selector: public css_base{
public:
	std::list<css_simple_selector*> simple_selectors_;

	css_selector();

	void addSimpleSelector(css_simple_selector* simple_selector);
	void addSelector(css_selector* selector);
	void prependSimpleSelector(css_simple_selector* simple_selector);
};

class css_selectors: public css_base{
public:
	std::list<css_selector*> selectors_;

	css_selectors();

	void addSelector(css_selector* selector);
};

class css_ruleset: public css_base{
public:
	std::list<css_selector*> selectors_;
	std::list<css_property*> properties_;

	css_ruleset();

	void addSelector(css_selector* selector);
	void setSelectors(std::list<css_selector*> &selectors);
	void addProperty(css_property* property);
	void addProperties(css_properties* properties);
};

class css_file: public css_base{
public:
	std::list<css_ruleset*> rulesets_;

	css_file();

	void addRuleset(css_ruleset* ruleset);
};

class css_function: public css_base{
public:
	const char* name_;
	std::list<css_base*> arguments_;

	css_function(const char* name);

	void addArgument(css_base* argument);
};

class css_hex: public css_base{
public:
	uint32_t value_;
	css_hex(uint32_t value);
	css_hex(const char *value);
};

class css_number: public css_base{
public:
	double value_;
	css_number(double value);
};

class css_percentage: public css_base{
public:
	double value_;
	css_percentage(double value);
};

class css_string: public css_base{
public:
	const char* value_;
	css_string(const char* value);
};

}

int parse_file(const char *filename);


#endif

