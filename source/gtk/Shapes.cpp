/*
 * Copyright (c) 2009-2020, Albertas Vyšniauskas
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

#include "Shapes.h"
#include <boost/math/special_functions/round.hpp>
namespace gtk {
void roundedRectangle(cairo_t *cr, float x, float y, float width, float height, float roundness) {
	float strength = 0.3f;
	cairo_new_path(cr);
	if (roundness == 0.0f) {
		cairo_move_to(cr, x, y);
		cairo_line_to(cr, x + width, y);
		cairo_line_to(cr, x + width, y + height);
		cairo_line_to(cr, x, y + height);
	} else {
		cairo_move_to(cr, x + roundness, y);
		cairo_line_to(cr, x + width - roundness, y);
		cairo_curve_to(cr, x + width - roundness * strength, y, x + width, y + roundness * strength, x + width, y + roundness);
		cairo_line_to(cr, x + width, y + height - roundness);
		cairo_curve_to(cr, x + width, y + height - roundness * strength, x + width - roundness * strength, y + height, x + width - roundness, y + height);
		cairo_line_to(cr, x + roundness, y + height);
		cairo_curve_to(cr, x + roundness * strength, y + height, x, y + height - roundness * strength, x, y + height - roundness);
		cairo_line_to(cr, x, y + roundness);
		cairo_curve_to(cr, x, y + roundness * strength, x + roundness * strength, y, x + roundness, y);
	}
	cairo_close_path(cr);
}
void splitRectangle(cairo_t *cr, float x, float y, float width, float height, float tilt) {
	cairo_new_path(cr);
	cairo_move_to(cr, x, y);
	cairo_line_to(cr, x + width / 2 + width * tilt / 2, y);
	cairo_line_to(cr, x + width / 2 - width * tilt / 2, y + height);
	cairo_line_to(cr, x, y + height);
	cairo_close_path(cr);
}
void setColor(cairo_t *cr, const Color &color) {
	using namespace boost::math;
	cairo_set_source_rgb(cr, round(color.rgb.red * 255.0) / 255.0, round(color.rgb.green * 255.0) / 255.0, round(color.rgb.blue * 255.0) / 255.0);
}
}
