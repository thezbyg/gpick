/*
 * Copyright (c) 2009-2017, Albertas Vyšniauskas
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

#ifndef GPICK_COLOR_SPACE_TYPE_H_
#define GPICK_COLOR_SPACE_TYPE_H_
#include "ColorSpaces.h"
#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>
namespace lua {
struct Script;
}
struct Color;
struct GlobalState;
struct ColorSpaceType {
	ColorSpace colorSpace;
	const char *name;
	int8_t channelCount;
	struct {
		const char *name, *shortName;
		double rawScale;
		double minValue;
		double maxValue;
		double step;
	} channels[5];
};
const ColorSpaceType *color_space_get_types();
const ColorSpaceType *color_space_get(ColorSpace colorSpace);
size_t color_space_count_types();
std::vector<std::string> color_space_color_to_text(const char *type, const Color &color, float alpha, lua::Script &script, GlobalState *gs);
#endif /* GPICK_COLOR_SPACE_TYPE_H_ */
