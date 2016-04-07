#include "parser/TextFile.h"
#include "Color.h"
#include <string.h>
#include <stdlib.h>
#include <cstddef>
#include <functional>
#include <vector>
#include <string>
#include <iostream>
using namespace std;

namespace text_file_parser {

class FSM
{
	public:
		int cs;
		char separator;
		int act;
		int top;
		char *ts;
		char *te;
		int stack[256];
		char buffer[8 * 1024];
		int line;
		int column;
		int line_start;
		int buffer_offset;
		int64_t number_i64;
		vector<int64_t> numbers_i64;
		char *number_double_start;
		vector<double> numbers_double;
		function<void(const Color&)> addColor;
		void handleNewline()
		{
			line++;
			column = 0;
			line_start = te - buffer;
		}
		int hexToInt(char hex)
		{
			if (hex >= '0' && hex <= '9') return hex - '0';
			if (hex >= 'a' && hex <= 'f') return hex - 'a' + 10;
			if (hex >= 'A' && hex <= 'F') return hex - 'A' + 10;
			return 0;
		}
		int hexPairToInt(const char *hex_pair)
		{
			return hexToInt(hex_pair[0]) << 4 | hexToInt(hex_pair[1]);
		}
		void colorHexFull(bool with_hash_symbol)
		{
			Color color;
			int start_index = with_hash_symbol ? 1 : 0;
			color.rgb.red = hexPairToInt(ts + start_index) / 255.0;
			color.rgb.green = hexPairToInt(ts + start_index + 2) / 255.0;
			color.rgb.blue = hexPairToInt(ts + start_index + 4) / 255.0;
			addColor(color);
		}
		void colorHexShort(bool with_hash_symbol)
		{
			Color color;
			int start_index = with_hash_symbol ? 1 : 0;
			color.rgb.red = hexToInt(ts[start_index + 0]) / 15.0;
			color.rgb.green = hexToInt(ts[start_index + 1]) / 15.0;
			color.rgb.blue = hexToInt(ts[start_index + 2]) / 15.0;
			color.ma[3] = 0;
			addColor(color);
		}
		void colorRgb()
		{
			Color color;
			color.rgb.red = numbers_i64[0] / 255.0;
			color.rgb.green = numbers_i64[1] / 255.0;
			color.rgb.blue = numbers_i64[2] / 255.0;
			color.ma[3] = 0;
			numbers_i64.clear();
			addColor(color);
		}
		void colorRgba()
		{
			Color color;
			color.rgb.red = numbers_i64[0] / 255.0;
			color.rgb.green = numbers_i64[1] / 255.0;
			color.rgb.blue = numbers_i64[2] / 255.0;
			color.ma[3] = numbers_double[0];
			numbers_i64.clear();
			numbers_double.clear();
			addColor(color);
		}
		void colorValues()
		{
			Color color;
			color.rgb.red = numbers_double[0];
			color.rgb.green = numbers_double[1];
			color.rgb.blue = numbers_double[2];
			color.ma[3] = 0;
			numbers_double.clear();
			addColor(color);
		}
		void colorValueIntegers()
		{
			Color color;
			color.rgb.red = numbers_i64[0] / 255.0;
			color.rgb.green = numbers_i64[1] / 255.0;
			color.rgb.blue = numbers_i64[2] / 255.0;
			color.ma[3] = 0;
			numbers_i64.clear();
			addColor(color);
		}
		double parseDouble(const char *start, const char *end)
		{
			string v(start, end);
			return stod(v.c_str());
		}
		void clearNumberStacks()
		{
			numbers_i64.clear();
			numbers_double.clear();
		}
};


%%{
	machine text_file;
	access fsm->;
	number_i64 = digit+ >{ fsm->number_i64 = 0; } ${ fsm->number_i64 = fsm->number_i64 * 10 + (*p - '0'); };
	sign = '-' | '+';
	number_double = sign? (([0-9]+ '.' [0-9]+) | ('.' [0-9]+) | ([0-9]+)) ('e'i sign? digit+)?;
	number = number_i64 %{ fsm->numbers_i64.push_back(fsm->number_i64); };
	real_number = number_double >{ fsm->number_double_start = p; } %{ fsm->numbers_double.push_back(fsm->parseDouble(fsm->number_double_start, p)); };

	newline = ('\n' | '\r\n') @{ fsm->handleNewline(); };
	anything = any | newline;
	multi_line_comment := anything* :>> '*/' @{ fgoto main; };
	single_line_comment := (any - newline)* :>> ('\n' | '\r\n') @{ fgoto main; };

	main := |*
		( '#'[0-9a-fA-F]{6} ) { if (configuration.full_hex) fsm->colorHexFull(true); };
		( '#'[0-9a-fA-F]{3} ) { if (configuration.short_hex) fsm->colorHexShort(true); };
		( [0-9a-fA-F]{6} ) { if (configuration.full_hex) fsm->colorHexFull(false); };
		( [0-9a-fA-F]{3} ) { if (configuration.short_hex) fsm->colorHexShort(false); };
		( 'rgb'i '(' space* number space* ',' space* number space* ',' space* number space* ')' ) { if (configuration.css_rgb) fsm->colorRgb(); else fsm->clearNumberStacks(); };
		( 'rgba'i '(' space* number space* ',' space* number space* ',' space* number space* ',' space* real_number space* ')' ) { if (configuration.css_rgba) fsm->colorRgba(); else fsm->clearNumberStacks(); };
		( number space* ',' space* number space* ',' space* number ) { if (configuration.int_values) fsm->colorValueIntegers(); else fsm->clearNumberStacks(); };
		( number space+ number space+ number ) { if (configuration.int_values) fsm->colorValueIntegers(); else fsm->clearNumberStacks(); };
		( real_number space* ',' space* real_number space* ',' space* real_number ) { if (configuration.float_values) fsm->colorValues(); else fsm->clearNumberStacks(); };
		( '//' ) { if (configuration.single_line_c_comments) fgoto single_line_comment; };
		( '/*' ) {  if (configuration.multi_line_c_comments) fgoto multi_line_comment; };
		( '#' ) { if (configuration.single_line_hash_comments) fgoto single_line_comment; };
		( space+ ) { };
		( punct+ ) { };
		( (any - (newline | space | punct | '//' | '/*'))+ ) { };
		( newline ) { };
		*|;
}%%

%% write data;

bool scanner(TextFile &text_file, const Configuration &configuration)
{
	FSM fsm_struct;
	FSM *fsm = &fsm_struct;
	fsm->ts = 0;
	fsm->te = 0;
	fsm->line = 0;
	fsm->line_start = 0;
	fsm->column = 0;
	fsm->buffer_offset = 0;
	bool parse_error = false;
	fsm->addColor = [&text_file](const Color &color){
		Color c = color;
		color_rgb_normalize(&c);
		text_file.addColor(c);
	};
	%% write init;
	int have = 0;
	while (1){
		char *p = fsm->buffer + have;
		int space = sizeof(fsm->buffer) - have;
		if (space == 0){
			text_file.outOfMemory();
			break;
		}
		char *eof = 0;
		auto read_size = text_file.read(fsm->buffer + have, space);
		char *pe = p + read_size;
		if (read_size > 0){
			if (read_size < sizeof(fsm->buffer)) eof = pe;
			%% write exec;
			if (fsm->cs == text_file_error) {
				parse_error = true;
				text_file.syntaxError(fsm->line, fsm->ts - fsm->buffer - fsm->line_start, fsm->line, fsm->te - fsm->buffer - fsm->line_start);
				break;
			}
			if (fsm->ts == 0){
				have = 0;
				fsm->line_start -= sizeof(fsm->buffer);
			}else{
				have = pe - fsm->ts;
				memmove(fsm->buffer, fsm->ts, have);
				int buffer_movement = fsm->ts - fsm->buffer;
				fsm->te -= buffer_movement;
				fsm->line_start -= buffer_movement;
				fsm->ts = fsm->buffer;
				fsm->buffer_offset += fsm->ts - fsm->buffer;
			}
		}else{
			break;
		}
	}
	return parse_error == false;
}

}
