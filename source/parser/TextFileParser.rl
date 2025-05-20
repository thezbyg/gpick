/*
 * Copyright (c) 2009-2022, Albertas Vyšniauskas
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

#include "parser/TextFile.h"
#include "Color.h"
#include "math/Algorithms.h"
#include <cstring>
#include <functional>
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include <cstdint>
#include <memory>
#include <utility>
namespace text_file_parser {
enum struct Unit {
	unitless,
	percentage,
	degree,
	gradian,
	turn,
	radian,
};
struct FSM {
	int cs;
	int act;
	char ws;
	char *ts, *te;
	char buffer[8 * 1024];
	int line, column, lineStart, bufferOffset;
	int64_t numberI64;
	std::vector<std::pair<int64_t, Unit>> numbersI64;
	char *numberStart;
	std::vector<std::pair<double, Unit>> numbersDouble;
	std::function<void(const Color&)> addColor;
	void handleNewline() {
		line++;
		column = 0;
		lineStart = te - buffer;
	}
	int hexToInt(char hex) {
		if (hex >= '0' && hex <= '9') return hex - '0';
		if (hex >= 'a' && hex <= 'f') return hex - 'a' + 10;
		if (hex >= 'A' && hex <= 'F') return hex - 'A' + 10;
		return 0;
	}
	int hexPairToInt(const char *hexPair) {
		return hexToInt(hexPair[0]) << 4 | hexToInt(hexPair[1]);
	}
	void colorHexFull(bool withHashSymbol) {
		Color color;
		int startIndex = withHashSymbol ? 1 : 0;
		color.red = hexPairToInt(ts + startIndex) / 255.0f;
		color.green = hexPairToInt(ts + startIndex + 2) / 255.0f;
		color.blue = hexPairToInt(ts + startIndex + 4) / 255.0f;
		color.alpha = 1;
		addColor(color);
	}
	void colorHexShort(bool withHashSymbol) {
		Color color;
		int startIndex = withHashSymbol ? 1 : 0;
		color.red = hexToInt(ts[startIndex + 0]) / 15.0f;
		color.green = hexToInt(ts[startIndex + 1]) / 15.0f;
		color.blue = hexToInt(ts[startIndex + 2]) / 15.0f;
		color.alpha = 1;
		addColor(color);
	}
	void colorHexWithAlphaFull(bool withHashSymbol) {
		Color color;
		int startIndex = withHashSymbol ? 1 : 0;
		color.red = hexPairToInt(ts + startIndex) / 255.0f;
		color.green = hexPairToInt(ts + startIndex + 2) / 255.0f;
		color.blue = hexPairToInt(ts + startIndex + 4) / 255.0f;
		color.alpha = hexPairToInt(ts + startIndex + 6) / 255.0f;
		addColor(color);
	}
	void colorHexWithAlphaShort(bool withHashSymbol) {
		Color color;
		int startIndex = withHashSymbol ? 1 : 0;
		color.red = hexToInt(ts[startIndex + 0]) / 15.0f;
		color.green = hexToInt(ts[startIndex + 1]) / 15.0f;
		color.blue = hexToInt(ts[startIndex + 2]) / 15.0f;
		color.alpha = hexToInt(ts[startIndex + 3]) / 15.0f;
		addColor(color);
	}
	float getPercentage(size_t index, double unitlessMultiplier = 1.0, double percentageMultiplier = 1.0) const {
		switch (numbersDouble[index].second) {
		case Unit::unitless:
			return static_cast<float>(numbersDouble[index].first * unitlessMultiplier);
		case Unit::percentage:
			return static_cast<float>(numbersDouble[index].first * (1 / 100.0) * percentageMultiplier);
		case Unit::degree:
		case Unit::turn:
		case Unit::radian:
		case Unit::gradian:
			break;
		}
		return 0;
	}
	static double normalizeDegrees(double value) {
		double tmp;
		double result = std::modf(value, &tmp);
		if (result < 0)
			result += 1.0;
		return result;
	}
	float getDegrees(size_t index) const {
		switch (numbersDouble[index].second) {
		case Unit::unitless:
		case Unit::degree:
			return static_cast<float>(normalizeDegrees(numbersDouble[index].first * (1 / 360.0)));
		case Unit::turn:
			return static_cast<float>(normalizeDegrees(numbersDouble[index].first));
		case Unit::radian:
			return static_cast<float>(normalizeDegrees(numbersDouble[index].first * (1 / math::PI / 2)));
		case Unit::gradian:
			return static_cast<float>(normalizeDegrees(numbersDouble[index].first * (1 / 400.0)));
		case Unit::percentage:
			break;
		}
		return 0;
	}
	void colorRgb() {
		Color color;
		color.red = getPercentage(0, 1.0 / 255);
		color.green = getPercentage(1, 1.0 / 255);
		color.blue = getPercentage(2, 1.0 / 255);
		color.alpha = numbersDouble.size() == 4 ? getPercentage(3) : 1;
		numbersDouble.clear();
		addColor(color);
	}
	void colorHsl() {
		Color color;
		color.hsl.hue = getDegrees(0);
		color.hsl.saturation = getPercentage(1);
		color.hsl.lightness = getPercentage(2);
		color.alpha = numbersDouble.size() == 4 ? getPercentage(3) : 1;
		numbersDouble.clear();
		addColor(color.normalizeRgb().hslToRgb());
	}
	void colorOklch() {
		Color color;
		color.oklch.L = getPercentage(0);
		color.oklch.C = getPercentage(1, 1.0, 0.4);
		color.oklch.h = getDegrees(2) * 360.0f;
		color.alpha = numbersDouble.size() == 4 ? getPercentage(3) : 1;
		numbersDouble.clear();
		addColor(color.oklchToRgb().normalizeRgb());
	}
	void colorValues() {
		Color color;
		color.red = static_cast<float>(numbersDouble[0].first);
		color.green = static_cast<float>(numbersDouble[1].first);
		color.blue = static_cast<float>(numbersDouble[2].first);
		if (numbersDouble.size() > 3)
			color.alpha = static_cast<float>(numbersDouble[3].first);
		else
			color.alpha = 1;
		numbersDouble.clear();
		addColor(color);
	}
	void colorValueIntegers() {
		Color color;
		color.red = numbersI64[0].first / 255.0f;
		color.green = numbersI64[1].first / 255.0f;
		color.blue = numbersI64[2].first / 255.0f;
		if (numbersI64.size() > 3)
			color.alpha = numbersI64[3].first / 255.0f;
		else
			color.alpha = 1;
		numbersI64.clear();
		addColor(color);
	}
	double parseDouble(const char *start, const char *end) {
		std::string v(start, end);
		try {
			return std::stod(v.c_str());
		} catch(...) {
			return 0;
		}
	}
	void clearNumberStacks() {
		numbersI64.clear();
		numbersDouble.clear();
	}
};

%%{
	machine text_file;
	access fsm.;
	numberI64 = digit+ >{ fsm.numberI64 = 0; } ${ fsm.numberI64 = fsm.numberI64 * 10 + (*p - '0'); };
	sign = '-' | '+';
	numberDouble = sign? (([0-9]+ '.' [0-9]+) | ('.' [0-9]+) | ([0-9]+)) ('e'i sign? digit+)?;
	integer = numberI64 %{ fsm.numbersI64.emplace_back(fsm.numberI64, Unit::unitless); };
	number = numberDouble >{ fsm.numberStart = p; } %{ fsm.numbersDouble.emplace_back(fsm.parseDouble(fsm.numberStart, p), Unit::unitless); };
	degrees = numberDouble >{ fsm.numberStart = p; } %{ fsm.numbersDouble.emplace_back(fsm.parseDouble(fsm.numberStart, p), Unit::unitless); } ('deg'i %{ fsm.numbersDouble.back().second = Unit::degree; } | 'grad'i %{ fsm.numbersDouble.back().second = Unit::gradian; } | 'turn'i %{ fsm.numbersDouble.back().second = Unit::turn; } | 'rad'i %{ fsm.numbersDouble.back().second = Unit::radian; })?;
	percentage = numberDouble >{ fsm.numberStart = p; } %{ fsm.numbersDouble.emplace_back(fsm.parseDouble(fsm.numberStart, p), Unit::percentage); } '%';
	numberOrPercentage = numberDouble >{ fsm.numberStart = p; } %{ fsm.numbersDouble.emplace_back(fsm.parseDouble(fsm.numberStart, p), Unit::unitless); } ('%' %{ fsm.numbersDouble.back().second = Unit::percentage; })?;
	newline = ('\n' | '\r\n') @{ fsm.handleNewline(); };
	ws = [ \t];
	separator = [,:;];

	action fullHexWithAlpha { configuration.fullHexWithAlpha }
	action fullHex { configuration.fullHex }
	action shortHexWithAlpha { configuration.shortHexWithAlpha }
	action shortHex { configuration.shortHex }
	action cssRgb { configuration.cssRgb }
	action cssRgba { configuration.cssRgba }
	action cssHsl { configuration.cssHsl }
	action cssHsla { configuration.cssHsla }
	action cssOklch { configuration.cssOklch }
	action cssOklchWithAlpha { configuration.cssOklch }
	action intValues { configuration.intValues }
	action floatValues { configuration.floatValues }
	action singleLineCComments { configuration.singleLineCComments }
	action multiLineCComments { configuration.multiLineCComments }
	action singleLineHashComments { configuration.singleLineHashComments }

	multiLineComment := |*
		( '*/' ) { fgoto main; };
		( any - newline ) { };
		( newline ) { };
		*|;
	singleLineComment := |*
		( newline ) { fgoto main; };
		( any - newline ) { };
		*|;
	main := |*
		( '#'[0-9a-fA-F]{8} ) when fullHexWithAlpha { fsm.colorHexWithAlphaFull(true); };
		( '#'[0-9a-fA-F]{6} ) when fullHex { fsm.colorHexFull(true); };
		( '#'[0-9a-fA-F]{4} ) when shortHexWithAlpha { fsm.colorHexWithAlphaShort(true); };
		( '#'[0-9a-fA-F]{3} ) when shortHex { fsm.colorHexShort(true); };
		( [0-9a-fA-F]{8} ) when fullHexWithAlpha { fsm.colorHexWithAlphaFull(false); };
		( [0-9a-fA-F]{6} ) when fullHex { fsm.colorHexFull(false); };
		( [0-9a-fA-F]{4} ) when shortHexWithAlpha { fsm.colorHexWithAlphaShort(false); };
		( [0-9a-fA-F]{3} ) when shortHex { fsm.colorHexShort(false); };
		( 'rgb'i '(' ws* numberOrPercentage ws+ numberOrPercentage ws+ numberOrPercentage ws* ')' ) when cssRgb { fsm.colorRgb(); };
		( 'rgb'i '(' ws* numberOrPercentage ws+ numberOrPercentage ws+ numberOrPercentage ws* '/' ws* numberOrPercentage ws* ')' ) when cssRgb { fsm.colorRgb(); };
		( 'rgb'i '(' ws* numberOrPercentage ws* ',' ws* numberOrPercentage ws* ',' ws* numberOrPercentage ws* ')' ) when cssRgb { fsm.colorRgb(); };
		( 'rgba'i '(' ws* numberOrPercentage ws+ numberOrPercentage ws+ numberOrPercentage ws* '/' ws* numberOrPercentage ws* ')' ) when cssRgba { fsm.colorRgb(); };
		( 'rgba'i '(' ws* numberOrPercentage ws* ',' ws* numberOrPercentage ws* ',' ws* numberOrPercentage ws* ',' ws* numberOrPercentage ws* ')' ) when cssRgba { fsm.colorRgb(); };
		( 'hsl'i '(' ws* degrees ws+ percentage ws+ percentage ws* ')' ) when cssHsl { fsm.colorHsl(); };
		( 'hsl'i '(' ws* degrees ws+ percentage ws+ percentage ws* '/' ws* numberOrPercentage ws*')' ) when cssHsl { fsm.colorHsl(); };
		( 'hsl'i '(' ws* degrees ws* ',' ws* percentage ws* ',' ws* percentage ws* ')' ) when cssHsl { fsm.colorHsl(); };
		( 'hsla'i '(' ws* degrees ws+ percentage ws+ percentage ws* '/' ws* numberOrPercentage ws* ')' ) when cssHsla { fsm.colorHsl(); };
		( 'hsla'i '(' ws* degrees ws* ',' ws* percentage ws* ',' ws* percentage ws* ',' ws* numberOrPercentage ws* ')' ) when cssHsla { fsm.colorHsl(); };
		( 'oklch'i '(' ws* numberOrPercentage ws+ numberOrPercentage ws+ degrees ws* ')' ) when cssOklch { fsm.colorOklch(); };
		( 'oklch'i '(' ws* numberOrPercentage ws+ numberOrPercentage ws+ degrees ws* '/' ws* numberOrPercentage ws* ')' ) when cssOklch { fsm.colorOklch(); };
		( integer ws* separator ws* integer ws* separator ws* integer (ws* separator ws* integer)? ) when intValues { fsm.colorValueIntegers(); };
		( integer ws+ integer ws+ integer (ws+ integer)? ) when intValues { fsm.colorValueIntegers(); };
		( number ws* separator ws* number ws* separator ws* number (ws* separator ws* number)? ) when floatValues { fsm.colorValues(); };
		( number ws+ number ws+ number (ws+ number)? ) when floatValues { fsm.colorValues(); };
		( '//' ) when singleLineCComments { fgoto singleLineComment; };
		( '/*' ) when multiLineCComments { fgoto multiLineComment; };
		( '#' ) when singleLineHashComments { fgoto singleLineComment; };
		( any - newline ) { fsm.clearNumberStacks(); };
		( newline ) { fsm.clearNumberStacks(); };
		*|;
}%%

%% write data;

bool scanner(TextFile &textFile, const Configuration &configuration) {
	FSM fsm = {};
	bool parseError = false;
	fsm.addColor = [&textFile](const Color &color) {
		textFile.addColor(color.normalizeRgb());
	};
	%% write init;
	int have = 0;
	while (1) {
		char *p = fsm.buffer + have;
		int ws = sizeof(fsm.buffer) - have;
		if (ws == 0) {
			textFile.outOfMemory();
			break;
		}
		char *eof = 0;
		auto readSize = textFile.read(fsm.buffer + have, ws);
		char *pe = p + readSize;
		if (readSize > 0) {
			if (readSize < sizeof(fsm.buffer))
				eof = pe;
			%% write exec;
			if (fsm.cs == text_file_error) {
				parseError = true;
				textFile.syntaxError(fsm.line, fsm.ts - fsm.buffer - fsm.lineStart, fsm.line, fsm.te - fsm.buffer - fsm.lineStart);
				break;
			}
			if (fsm.ts == 0) {
				have = 0;
				fsm.lineStart -= sizeof(fsm.buffer);
			} else {
				have = pe - fsm.ts;
				std::memmove(fsm.buffer, fsm.ts, have);
				int bufferMovement = fsm.ts - fsm.buffer;
				fsm.te -= bufferMovement;
				fsm.lineStart -= bufferMovement;
				fsm.ts = fsm.buffer;
				fsm.bufferOffset += fsm.ts - fsm.buffer;
			}
		} else {
			break;
		}
	}
	return parseError == false;
}
}
