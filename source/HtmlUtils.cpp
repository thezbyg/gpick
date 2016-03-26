/*
 * Copyright (c) 2009-2016, Albertas Vyšniauskas
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

#include "HtmlUtils.h"
#include "Color.h"
#include <algorithm>
#include <boost/math/special_functions/round.hpp>
using namespace std;

template<typename InputIterator, typename OutputIterator>
OutputIterator copy_zero_terminated_string(InputIterator begin, OutputIterator out)
{
	while (*begin != '\0'){
		*out++ = *begin++;
	}
	return out;
}
template<typename InputIterator, typename OutputIterator>
OutputIterator escape(InputIterator begin, InputIterator end, OutputIterator out)
{
	const char escape_chars[] = {'&', '"', '\'', '<', '>'};
	const char *replacements[] = {"&amp;", "&quot;", "&apos;", "&lt;", "&gt;"};
	const size_t n_escape_chars = sizeof(escape_chars) / sizeof(const char);
	for (; begin != end; ++begin){
		size_t escape_char_index = distance(escape_chars, find(escape_chars, escape_chars + n_escape_chars, *begin));
		if (escape_char_index != n_escape_chars){
			out = copy_zero_terminated_string(replacements[escape_char_index], out);
		}else{
			*out++ = *begin;
		}
	}
	return out;
}
string &escapeHtmlInplace(string &str)
{
	string result;
	result.reserve(str.size());
	escape(str.begin(), str.end(), back_inserter(result));
	str.swap(result);
	return str;
}
string escapeHtml(const string &str)
{
	string result;
	result.reserve(str.size());
	escape(str.begin(), str.end(), back_inserter(result));
	return result;
}
std::ostream& operator<<(std::ostream& os, const HtmlRGB color)
{
	using boost::math::iround;
	int r, g, b;
	r = iround(color.color->rgb.red * 255);
	g = iround(color.color->rgb.green * 255);
	b = iround(color.color->rgb.blue * 255);
	auto flags = os.flags();
	os << "rgb(" << dec << r << ", " << g << ", " << b << ")";
	os.setf(flags);
	return os;
}
std::ostream& operator<<(std::ostream& os, const HtmlHEX color)
{
	using boost::math::iround;
	int r, g, b;
	r = iround(color.color->rgb.red * 255);
	g = iround(color.color->rgb.green * 255);
	b = iround(color.color->rgb.blue * 255);
	char fill = os.fill();
	auto flags = os.flags();
	os << "#" << hex << setfill('0') << setw(2) << r << setw(2) << g << setw(2) << b << setfill(fill);
	os.setf(flags);
	return os;
}
std::ostream& operator<<(std::ostream& os, const HtmlHSL color)
{
	using boost::math::iround;
	int h, s, l;
	h = iround(color.color->hsl.hue * 360);
	s = iround(color.color->hsl.saturation * 100);
	l = iround(color.color->hsl.lightness * 100);
	auto flags = os.flags();
	os << "hsl(" << dec << h << ", " << s << "%, " << l << "%)";
	os.setf(flags);
	return os;
}

