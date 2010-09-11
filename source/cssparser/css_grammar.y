
%include {

#include <css_parser.h>
#include "css_lex.h"
#include <assert.h>

using namespace css_parser;

}

%extra_argument { parse_parm_s *parm }
%name css_parse
%token_prefix CSS_TOKEN_

%token_type {css_yystype}
%default_type {css_yystype}


%left S.

%right IDENT.

%left CDO CDC.
%left INCLUDES.
%left DASHMATCH.
%left STRING.
%left INVALID.
%left HEXCOLOR.

%left SEMICOLON COMMA LBRACE RBRACE HASH GREATER PLUS CLPARENTH RBRACK SLASH.
%nonassoc DOT COLON LBRACK ASTERISK.

%left IMPORT_SYM.
%left PAGE_SYM.
%left MEDIA_SYM.
%left CHARSET_SYM.
%left IMPORTANT_SYM.
%left FONTFACE_SYM.
%left NAMESPACE_SYM.

%left EMS EXS LENGTH ANGLE TIME FREQ DIMENSION PERCENTAGE NUMBER.
%left FUNCTION.
%left URI.

%type property {css_property*}
%type expr {css_base*}
%type term {css_base*}
%type hexcolor {css_hex*}
%type function {css_function*}
%type declaration {css_property*}
%type selectors {css_selectors*}
%type maybe_declarations {css_properties*}
%type ruleset {css_ruleset*}
%type declarations {css_properties*}
%type declaration_list {css_properties*}
%type selector {css_selector*}
%type universal_selector {css_simple_selector*}
%type type_selector {css_simple_selector*}
%type class_selector {css_simple_selector*}
%type id_selector {css_simple_selector*}
%type simple_selectors {css_selector*}
%type pseudo_selector {css_simple_selector*}
%type attribute_selector {css_simple_selector*}
%type simple_selector {css_selector*}

%syntax_error {
	printf("Error: %s\n", yyTokenName[yymajor]);
}

%start_symbol stylesheet



stylesheet ::= maybe_charset maybe_sgml maybe_rules.

maybe_space ::= .
maybe_space ::= maybe_space S.

space ::= S.
space ::= space S.

maybe_sgml ::= .
maybe_sgml ::= maybe_sgml CDO.
maybe_sgml ::= maybe_sgml CDC.
maybe_sgml ::= maybe_sgml S.

maybe_charset ::= .
maybe_charset ::= CHARSET_SYM maybe_space STRING maybe_space SEMICOLON.

maybe_rules ::= .
maybe_rules ::= maybe_rules rule maybe_sgml.


rule ::= import.
rule ::= ruleset.
rule ::= media.
rule ::= page.

medium ::= IDENT maybe_space.

maybe_medium ::= .
maybe_medium ::= medium.
maybe_medium ::= maybe_medium COMMA maybe_space medium.


uri_or_string ::= URI.
uri_or_string ::= STRING.

import ::= IMPORT_SYM maybe_space uri_or_string maybe_space maybe_medium SEMICOLON.
import ::= IMPORT_SYM error SEMICOLON.

ruleset(R) ::= selectors(A) block_start maybe_declarations(B) block_end. {
	css_ruleset *ruleset = new(parm) css_ruleset(); 
	ruleset->setSelectors(A->selectors_);
	if (B) ruleset->addProperties(B);
	R = ruleset;

	parm->page->addRuleset(ruleset);
}

ruleset(R) ::= selectors(A) block_start space maybe_declarations(B) block_end. {
	css_ruleset *ruleset = new(parm) css_ruleset(); 
	ruleset->setSelectors(A->selectors_);
	if (B) ruleset->addProperties(B);
	R = ruleset;

	parm->page->addRuleset(ruleset);
}

media_list ::= media_list COMMA maybe_space medium.
media_list ::= medium.

media ::= MEDIA_SYM maybe_space media_list block_start maybe_space maybe_declarations block_end.
media ::= MEDIA_SYM maybe_space block_start maybe_space maybe_declarations block_end.

page ::= PAGE_SYM maybe_space maybe_pseudo_page block_start maybe_space maybe_declarations block_end.

maybe_pseudo_page ::= .
maybe_pseudo_page ::= COLON IDENT maybe_space.

maybe_declarations(R) ::= . { R = 0; }
maybe_declarations(R) ::= declarations(A). { R = A; }

universal_selector(R) ::= ASTERISK(A). { R = new(parm) css_simple_selector(A.string); }
type_selector(R) ::= IDENT(A). { R = new(parm) css_simple_selector(A.string); }
class_selector(R) ::= DOT IDENT(A). { R = new(parm) css_simple_selector(A.string); }
id_selector(R) ::= HASH IDENT(A). { R = new(parm) css_simple_selector(A.string); }

attribute_selector(R) ::= LBRACK maybe_space IDENT(A) maybe_space RBRACK. { R = new(parm) css_simple_selector(A.string); }

pseudo_selector(R) ::= COLON IDENT(A). { R = new(parm) css_simple_selector(A.string); }
pseudo_selector ::= COLON FUNCTION maybe_space CLPARENTH.
pseudo_selector ::= COLON FUNCTION maybe_space IDENT maybe_space CLPARENTH.
pseudo_selector(R) ::= COLON COLON IDENT(A). { R = new(parm) css_simple_selector(A.string); }

simple_selectors(R) ::= simple_selectors(A) id_selector(B). { A->addSimpleSelector(B); R = A; }
simple_selectors(R) ::= simple_selectors(A) class_selector(B). { A->addSimpleSelector(B); R = A; }
simple_selectors(R) ::= simple_selectors(A) attribute_selector(B). { A->addSimpleSelector(B); R = A; }
simple_selectors(R) ::= simple_selectors(A) pseudo_selector(B). { A->addSimpleSelector(B); R = A; }
simple_selectors(R) ::= id_selector(A). {
	css_selector* selector = new(parm) css_selector(); 
	selector->addSimpleSelector(A);
	R = selector;
}
simple_selectors(R) ::= class_selector(A). {
	css_selector* selector = new(parm) css_selector(); 
	selector->addSimpleSelector(A);
	R = selector;
}
simple_selectors(R) ::= attribute_selector(A). {
	css_selector* selector = new(parm) css_selector(); 
	selector->addSimpleSelector(A);
	R = selector;
}
simple_selectors(R) ::= pseudo_selector(A). {
	css_selector* selector = new(parm) css_selector(); 
	selector->addSimpleSelector(A);
	R = selector;
}

simple_selector(R) ::= type_selector(A). {
	css_selector* selector = new(parm) css_selector(); 
	selector->addSimpleSelector(A);
	R = selector;
}
simple_selector(R) ::= type_selector(A) simple_selectors(B). {
	B->prependSimpleSelector(A);
	R = B;
}
simple_selector(R) ::= universal_selector(A). {
	css_selector* selector = new(parm) css_selector(); 
	selector->addSimpleSelector(A);
	R = selector;
}
simple_selector(R) ::= universal_selector(A) simple_selectors(B). {
	B->prependSimpleSelector(A);
	R = B;
}
simple_selector(R) ::= simple_selectors(A). {
	R = A;
}


selector(R) ::= simple_selector(A). { R = A; }
selector(R) ::= selector(A) combinator simple_selector(B). { 
	A->addSelector(B);
	R = A;
}
selector(R) ::= selector(A) space simple_selector(B). {
	A->addSelector(B);
	R = A;
}
selector(R) ::= selector(A) space combinator simple_selector(B). {
	A->addSelector(B);
	R = A;
}

selectors(R) ::= selector(A). {
	css_selectors* selectors = new(parm) css_selectors(); 
	selectors->addSelector(A);
	R = selectors;
}

selectors(R) ::= selector(A) space. {
	css_selectors* selectors = new(parm) css_selectors(); 
	selectors->addSelector(A);
	R = selectors;
}

selectors(R) ::= selectors(A) COMMA maybe_space selector(B). {
	A->addSelector(B);
	R = A;
}

selectors(R) ::= selectors(A) COMMA maybe_space selector(B) space. {
	A->addSelector(B);
	R = A;
}


combinator ::= PLUS maybe_space.
combinator ::= GREATER maybe_space.

declaration_list(R) ::= declaration(A) SEMICOLON maybe_space. {
	css_properties* properties = new(parm) css_properties(); 
	if (A) properties->addProperty(A);
	R = properties;
}
declaration_list(R) ::= declaration_list(A) declaration(B) SEMICOLON maybe_space. {
	if (B) A->addProperty(B);
	R = A;
}

declarations(R) ::= declaration(A). {
	css_properties* properties = new(parm) css_properties(); 
	if (A) properties->addProperty(A);
	R = properties;
}
declarations(R) ::= declaration_list(A) declaration(B). {
	if (B) A->addProperty(B);
	R = A;
}
declarations(R) ::= declaration_list(A). {
	R = A;
}

declaration(R) ::= property(A) COLON maybe_space expr(B) maybe_prio. {
  if (strcmp(A->name_, "color") == 0 || strcmp(A->name_, "background-color") == 0){
		A->addValue(B);
		R = A;
	}else{
		R = 0;
	}
}

property(R) ::= IDENT(A) maybe_space.		{ R = new(parm) css_property(A.string);}

maybe_prio ::= .
maybe_prio ::= IMPORTANT_SYM maybe_space.


expr(R) ::= term(A).						{ R = A; }
expr(R) ::= expr(A) term(B).				{ R = A; R = B; }
expr(R) ::= expr(A) operator term(C).		{ R = A; R = C; }

unary_operator ::= MINUS.
unary_operator ::= PLUS.

maybe_unary_operator ::= .
maybe_unary_operator ::= unary_operator.

operator ::= SLASH maybe_space.
operator ::= COMMA maybe_space.

term(R) ::= maybe_unary_operator NUMBER(A) maybe_space.		{ R = new(parm) css_number(A.number); }
term(R) ::= maybe_unary_operator PERCENTAGE maybe_space.	{ R = 0; }
term(R) ::= maybe_unary_operator LENGTH maybe_space.		{ R = 0; }
term(R) ::= maybe_unary_operator EMS maybe_space.			{ R = 0; }
term(R) ::= maybe_unary_operator EXS maybe_space.			{ R = 0; }
term(R) ::= maybe_unary_operator ANGLE maybe_space.			{ R = 0; }
term(R) ::= maybe_unary_operator TIME maybe_space.			{ R = 0; }
term(R) ::= maybe_unary_operator FREQ maybe_space.			{ R = 0; }
term(R) ::= STRING(A) maybe_space.							{ R = new(parm) css_string(A.string); }
term(R) ::= IDENT(A) maybe_space.							{ R = new(parm) css_string(A.string); }
term(R) ::= URI maybe_space.								{ R = 0; }
term(R) ::= HEXCOLOR(A) maybe_space.						{ R = new(parm) css_hex(A.string); }
term(R) ::= function(A) maybe_space.						{ R = A; }

  
function(R) ::= FUNCTION(A) maybe_space expr(B) CLPARENTH maybe_space. {
	css_function* function = new(parm) css_function(A.string); 
	function->addArgument(B);
	R = function;
}

block_start ::= LBRACE.

block_end ::= RBRACE.
block_end ::= error RBRACE.

