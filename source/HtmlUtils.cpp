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
#include <iterator>
#include <boost/math/special_functions/round.hpp>

template<typename InputIterator, typename OutputIterator>
OutputIterator copy_zero_terminated_string(InputIterator begin, OutputIterator out) {
	while (*begin != '\0'){
		*out++ = *begin++;
	}
	return out;
}
template<typename InputIterator, typename OutputIterator>
OutputIterator escape(InputIterator begin, InputIterator end, OutputIterator out) {
	const char escapeChars[] = {'&', '"', '\'', '<', '>'};
	const char *replacements[] = {"&amp;", "&quot;", "&apos;", "&lt;", "&gt;"};
	const size_t escapeCharCount = sizeof(escapeChars) / sizeof(const char);
	for (; begin != end; ++begin){
		size_t escapeCharIndex = std::distance(escapeChars, std::find(escapeChars, escapeChars + escapeCharCount, *begin));
		if (escapeCharIndex != escapeCharCount){
			out = copy_zero_terminated_string(replacements[escapeCharIndex], out);
		}else{
			*out++ = *begin;
		}
	}
	return out;
}
HtmlRGB::HtmlRGB(const Color &color):
	color(color) {
}
HtmlRGBA::HtmlRGBA(const Color &color):
	color(color) {
}
HtmlHEX::HtmlHEX(const Color &color):
	color(color) {
}
HtmlHSL::HtmlHSL(const Color &color):
	color(color) {
}
HtmlHSLA::HtmlHSLA(const Color &color):
	color(color) {
}
std::string &escapeHtmlInplace(std::string &str) {
	std::string result;
	result.reserve(str.size());
	escape(str.begin(), str.end(), std::back_inserter(result));
	str.swap(result);
	return str;
}
std::string escapeHtml(const std::string &str) {
	std::string result;
	result.reserve(str.size());
	escape(str.begin(), str.end(), std::back_inserter(result));
	return result;
}
std::ostream& operator<<(std::ostream& os, const HtmlRGB color) {
	using boost::math::iround;
	int r, g, b;
	r = iround(color.color.rgb.red * 255);
	g = iround(color.color.rgb.green * 255);
	b = iround(color.color.rgb.blue * 255);
	auto flags = os.flags();
	os << "rgb(" << std::dec << r << ", " << g << ", " << b << ")";
	os.setf(flags);
	return os;
}
std::ostream& operator<<(std::ostream& os, const HtmlRGBA color) {
	using boost::math::iround;
	int r, g, b;
	r = iround(color.color.rgb.red * 255);
	g = iround(color.color.rgb.green * 255);
	b = iround(color.color.rgb.blue * 255);
	auto flags = os.flags();
	os << "rgba(" << std::dec << r << ", " << g << ", " << b << ", " << std::setprecision(3) << color.color.alpha << ")";
	os.setf(flags);
	return os;
}
std::ostream& operator<<(std::ostream& os, const HtmlHEX color) {
	using boost::math::iround;
	int r, g, b;
	r = iround(color.color.rgb.red * 255);
	g = iround(color.color.rgb.green * 255);
	b = iround(color.color.rgb.blue * 255);
	char fill = os.fill();
	auto flags = os.flags();
	os << "#" << std::hex << std::setfill('0') << std::setw(2) << r << std::setw(2) << g << std::setw(2) << b << std::setfill(fill);
	os.setf(flags);
	return os;
}
std::ostream& operator<<(std::ostream& os, const HtmlHSL color) {
	using boost::math::iround;
	int h, s, l;
	h = iround(color.color.hsl.hue * 360);
	s = iround(color.color.hsl.saturation * 100);
	l = iround(color.color.hsl.lightness * 100);
	auto flags = os.flags();
	os << "hsl(" << std::dec << h << ", " << s << "%, " << l << "%)";
	os.setf(flags);
	return os;
}
std::ostream& operator<<(std::ostream& os, const HtmlHSLA color) {
	using boost::math::iround;
	int h, s, l;
	h = iround(color.color.hsl.hue * 360);
	s = iround(color.color.hsl.saturation * 100);
	l = iround(color.color.hsl.lightness * 100);
	auto flags = os.flags();
	os << "hsla(" << std::dec << h << ", " << s << "%, " << l << "%, " << std::setprecision(3) << color.color.alpha << ")";
	os.setf(flags);
	return os;
}
