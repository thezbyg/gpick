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

#ifndef GPICK_HTML_UTILS_H_
#define GPICK_HTML_UTILS_H_
#include <string>
#include <iosfwd>
std::string &escapeHtmlInplace(std::string &str);
std::string escapeHtml(const std::string &str);
struct Color;
struct HtmlRGB {
	HtmlRGB(const Color &color);
	const Color &color;
};
struct HtmlRGBA {
	HtmlRGBA(const Color &color);
	const Color &color;
};
struct HtmlHEX {
	HtmlHEX(const Color &color);
	const Color &color;
};
struct HtmlHSL {
	HtmlHSL(const Color &color);
	const Color &color;
};
struct HtmlHSLA {
	HtmlHSLA(const Color &color);
	const Color &color;
};
std::ostream& operator<<(std::ostream& os, const HtmlRGB color);
std::ostream& operator<<(std::ostream& os, const HtmlRGBA color);
std::ostream& operator<<(std::ostream& os, const HtmlHEX color);
std::ostream& operator<<(std::ostream& os, const HtmlHSL color);
std::ostream& operator<<(std::ostream& os, const HtmlHSLA color);
#endif /* GPICK_HTML_UTILS_H_ */
