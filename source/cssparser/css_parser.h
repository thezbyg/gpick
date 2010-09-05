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

#ifndef CSS_PARSER_H
#define CSS_PARSER_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "memory_manager.h"

typedef struct parse_parm_s parse_parm;

class css_base{
public:
	uint32_t struct_type;
	struct css_base *next;
	struct css_base *end;

	css_base(uint32_t id):struct_type(id),next(0),end(this){};
	css_base():struct_type(0),next(0),end(this){};

	void* operator new(size_t num_bytes, parse_parm* parm);
	void operator delete(void* base, parse_parm* parm);
};


typedef struct parse_parm_s{
	css_base *page;
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
	struct css_base *base;
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



#define CSS_PARSER_PROPERTY			0x0001
#define CSS_PARSER_FUNCTION			0x0002
#define CSS_PARSER_HEX				0x0003
#define CSS_PARSER_NUMBER			0x0004
#define CSS_PARSER_PERCENTAGE		0x0005
#define CSS_PARSER_STRING			0x0006


css_base* css_parser_push_base(css_base* base, css_base* x);

class css_property: public css_base{
public:
	char* name;
	css_property(char* _name):css_base(CSS_PARSER_PROPERTY),name(_name){};
};

class css_function: public css_base{
public:
	char* name;
	css_function(char* _name):css_base(CSS_PARSER_FUNCTION),name(_name){};
};

class css_hex: public css_base{
public:
	uint32_t value;
	css_hex(uint32_t _value):css_base(CSS_PARSER_HEX),value(_value){};
};

class css_number: public css_base{
public:
	double value;
	css_number(double _value):css_base(CSS_PARSER_NUMBER),value(_value){};
};

class css_percentage: public css_base{
public:
	double value;
	css_percentage(double _value):css_base(CSS_PARSER_PERCENTAGE),value(_value){};
};

class css_string: public css_base{
public:
	char* value;
	css_string(char* _value):css_base(CSS_PARSER_STRING),value(_value){};
};

int parse_file(const char *filename);


#endif
