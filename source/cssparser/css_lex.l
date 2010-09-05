
%option reentrant bison-bridge
%option noyywrap nounput batch
%option never-interactive
%option prefix="css_"
%option case-insensitive
%option stack
%option 8bit
%option nounistd

%option noyyalloc noyyrealloc noyyfree


%{
#include "css_grammar.h"
#include <css_parser.h>
#include "memory_manager.h"

using namespace std;

#define YY_USER_ACTION  { yyextra->last_column += yyleng; yyextra->position += yyleng; }

char* css_strdup(char* x, void * yyscanner);


%}


h			[0-9a-f]
nonascii	[\200-\377]
unicode		\\{h}{1,6}(\r\n|[ \t\r\n\f])?
escape		{unicode}|\\[^\r\n\f0-9a-f]
nmstart		[_a-z]|{nonascii}|{escape}
nmchar		[_a-z0-9-]|{nonascii}|{escape}
string1		\"([^\n\r\f\\"]|\\{nl}|{escape})*\"
string2		\'([^\n\r\f\\']|\\{nl}|{escape})*\'
invalid1	\"([^\n\r\f\\"]|\\{nl}|{escape})*
invalid2	\'([^\n\r\f\\']|\\{nl}|{escape})*

comment_begin	"/*"
comment_end		"*/"

ident		-?{nmstart}{nmchar}*
name		{nmchar}+
num		[0-9]+|[0-9]*"."[0-9]+
string		{string1}|{string2}
invalid		{invalid1}|{invalid2}
url		([!#$%&*-~]|{nonascii}|{escape})*
s		[ \t\r\n\f]+
w		{s}?
ws		[ \t]+
nl		\n|\r\n|\r|\f
hexcolor	{h}{3}|{h}{6}

A		a|\\0{0,4}(41|61)(\r\n|[ \t\r\n\f])?
B		b|\\0{0,4}(42|62)(\r\n|[ \t\r\n\f])?
C		c|\\0{0,4}(43|63)(\r\n|[ \t\r\n\f])?
D		d|\\0{0,4}(44|64)(\r\n|[ \t\r\n\f])?
E		e|\\0{0,4}(45|65)(\r\n|[ \t\r\n\f])?
F		f|\\0{0,4}(46|66)(\r\n|[ \t\r\n\f])?
G		g|\\0{0,4}(47|67)(\r\n|[ \t\r\n\f])?|\\g
H		h|\\0{0,4}(48|68)(\r\n|[ \t\r\n\f])?|\\h
I		i|\\0{0,4}(49|69)(\r\n|[ \t\r\n\f])?|\\i
J		j|\\0{0,4}(4a|6a)(\r\n|[ \t\r\n\f])?|\\j
K		k|\\0{0,4}(4b|6b)(\r\n|[ \t\r\n\f])?|\\k
L       l|\\0{0,4}(4c|6c)(\r\n|[ \t\r\n\f])?|\\l
M		m|\\0{0,4}(4d|6d)(\r\n|[ \t\r\n\f])?|\\m
N		n|\\0{0,4}(4e|6e)(\r\n|[ \t\r\n\f])?|\\n
O		o|\\0{0,4}(4f|6f)(\r\n|[ \t\r\n\f])?|\\o
P		p|\\0{0,4}(50|70)(\r\n|[ \t\r\n\f])?|\\p
Q		q|\\0{0,4}(51|71)(\r\n|[ \t\r\n\f])?|\\q
R		r|\\0{0,4}(52|72)(\r\n|[ \t\r\n\f])?|\\r
S		s|\\0{0,4}(53|73)(\r\n|[ \t\r\n\f])?|\\s
T		t|\\0{0,4}(54|74)(\r\n|[ \t\r\n\f])?|\\t
U       u|\\0{0,4}(55|75)(\r\n|[ \t\r\n\f])?|\\u
V       v|\\0{0,4}(56|76)(\r\n|[ \t\r\n\f])?|\\v
W       w|\\0{0,4}(57|77)(\r\n|[ \t\r\n\f])?|\\w
X		x|\\0{0,4}(58|78)(\r\n|[ \t\r\n\f])?|\\x
Y       y|\\0{0,4}(59|79)(\r\n|[ \t\r\n\f])?|\\y
Z		z|\\0{0,4}(5a|7a)(\r\n|[ \t\r\n\f])?|\\z

%x comment
%x sgml

%%

%{
	CSS_LOCATION_STEP (*yyextra);
%}


{nl}+			{CSS_LOCATION_LINES (*yyextra, yyleng); CSS_LOCATION_STEP (*yyextra); return CSS_TOKEN_S;}

{ws}			{CSS_LOCATION_STEP (*yyextra); return CSS_TOKEN_S;}

{comment_begin}			{ yy_push_state(comment, yyscanner); }
<comment>{nl}+			{CSS_LOCATION_LINES (*yyextra, yyleng); CSS_LOCATION_STEP (*yyextra); }
<comment>[^*\n\r\f]*		/* eat up comments */
<comment>"*"+[^*/\n\r\f]*   /* eat up '*'s not followed by '/'s */
<comment>{comment_end}	{ yy_pop_state(yyscanner); }

"<!--"					{ yy_push_state(sgml, yyscanner); }
<sgml>{nl}+				{CSS_LOCATION_LINES (*yyextra, yyleng); CSS_LOCATION_STEP (*yyextra); }
<sgml>[^-\n\r\f]*		/* eat all until '-' */
<sgml>"--"+[^->\n\r\f]*	/* eat all '--'s not followed by '>'s */
<sgml>"-"+[^->\n\r\f]*	/* eat all '-'s not followed by '-'s */ 
<sgml>"-->"				{ yy_pop_state(yyscanner); }

"~="			{return CSS_TOKEN_INCLUDES;}
"|="			{return CSS_TOKEN_DASHMATCH;}

{string}		{yylval->string = css_strdup(yytext, yyscanner); return CSS_TOKEN_STRING;}
{invalid}		{return CSS_TOKEN_INVALID; /* unclosed string */}

{ident}			{yylval->string = css_strdup(yytext, yyscanner); return CSS_TOKEN_IDENT; }

"#"{hexcolor}		{yylval->string = css_strdup(yytext, yyscanner); return CSS_TOKEN_HEXCOLOR;}

"@import"		{return CSS_TOKEN_IMPORT_SYM;}
"@page"			{return CSS_TOKEN_PAGE_SYM;}
"@media"		{return CSS_TOKEN_MEDIA_SYM;}
"@font-face"	{return CSS_TOKEN_FONTFACE_SYM;}
"@charset "		{return CSS_TOKEN_CHARSET_SYM;}
"@namespace"	{return CSS_TOKEN_NAMESPACE_SYM;}

"!"({w}|{comment_begin})*{I}{M}{P}{O}{R}{T}{A}{N}{T}	{return CSS_TOKEN_IMPORTANT_SYM;}

{num}{E}{M}		{return CSS_TOKEN_EMS;}
{num}{E}{X}		{return CSS_TOKEN_EXS;}
{num}{P}{X}		{return CSS_TOKEN_LENGTH;}
{num}{C}{M}		{return CSS_TOKEN_LENGTH;}
{num}{M}{M}		{return CSS_TOKEN_LENGTH;}
{num}{I}{N}		{return CSS_TOKEN_LENGTH;}
{num}{P}{T}		{return CSS_TOKEN_LENGTH;}
{num}{P}{C}		{return CSS_TOKEN_LENGTH;}
{num}{D}{E}{G}		{return CSS_TOKEN_ANGLE;}
{num}{R}{A}{D}		{return CSS_TOKEN_ANGLE;}
{num}{G}{R}{A}{D}	{return CSS_TOKEN_ANGLE;}
{num}{M}{S}		{return CSS_TOKEN_TIME;}
{num}{S}		{return CSS_TOKEN_TIME;}
{num}{H}{Z}		{return CSS_TOKEN_FREQ;}
{num}{K}{H}{Z}		{return CSS_TOKEN_FREQ;}
{num}{ident}		{return CSS_TOKEN_DIMENSION;}

{num}%			{yylval->number = strtod(yytext, 0); return CSS_TOKEN_PERCENTAGE;}
{num}			{yylval->number = strtod(yytext, 0); return CSS_TOKEN_NUMBER;}

{U}{R}{L}"("{w}{string}{w}")"	{return CSS_TOKEN_URI;}
{U}{R}{L}"("{w}{url}{w}")"	{return CSS_TOKEN_URI;}

{ident}"("		{yylval->string = css_strdup(yytext, yyscanner); return CSS_TOKEN_FUNCTION;}

"-"			{return CSS_TOKEN_MINUS;}
"+"			{return CSS_TOKEN_PLUS;}
"/"			{return CSS_TOKEN_SLASH;}
"*"			{return CSS_TOKEN_ASTERISK;}
"#"			{return CSS_TOKEN_HASH;}
"{"			{return CSS_TOKEN_LBRACE;}
"}"			{return CSS_TOKEN_RBRACE;}
"["			{return CSS_TOKEN_LBRACK;}
"]"			{return CSS_TOKEN_RBRACK;}
"."			{return CSS_TOKEN_DOT;}
","			{return CSS_TOKEN_COMMA;}
":"			{return CSS_TOKEN_COLON;}
";"			{return CSS_TOKEN_SEMICOLON;}
")"			{return CSS_TOKEN_CLPARENTH;}
">"			{return CSS_TOKEN_GREATER;}

.			{printf("Unknown char\n");
return *yytext;}

%%


 void * css_alloc (size_t bytes, void* yyscanner) {
	struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;
	return memory_alloc (yyextra->memory, bytes);
 }
 
 void * css_realloc (void * ptr, size_t bytes, void* yyscanner) {
	struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;
	return memory_realloc (yyextra->memory, ptr, bytes);
 }
 
 void css_free (void * ptr, void * yyscanner) {  
	struct yyguts_t * yyg = (struct yyguts_t*)yyscanner;
	memory_free(yyextra->memory, ptr);
 }
 
 char* css_strdup(char* x, void * yyscanner){
	uint32_t size=strlen(x)+1;
	char* r=(char*)css_alloc(size, yyscanner);
	memcpy(r, x, size);
	return r;
 }
 


 