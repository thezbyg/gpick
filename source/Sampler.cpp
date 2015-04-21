/*
 * Copyright (c) 2009-2012, Albertas Vyšniauskas
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


#include "Sampler.h"
#include "MathUtil.h"
#include <math.h>
#include <gdk/gdk.h>

using namespace math;

struct Sampler{
	int oversample;
	enum SamplerFalloff falloff;
	float (*falloff_fnc)(float distance);
	struct ScreenReader* screen_reader;
};

static float sampler_falloff_none(float distance) {
	return 1;
}

static float sampler_falloff_linear(float distance) {
	return 1-distance;
}

static float sampler_falloff_quadratic(float distance) {
	return 1-(distance*distance);
}

static float sampler_falloff_cubic(float distance) {
	return 1-(distance*distance*distance);
}

static float sampler_falloff_exponential(float distance) {
	return 1/exp(5*distance*distance);
}

struct Sampler* sampler_new(struct ScreenReader* screen_reader) {
	struct Sampler* sampler = new struct Sampler;
	sampler->oversample=0;
	sampler_set_falloff(sampler, NONE);
	sampler->screen_reader = screen_reader;
	return sampler;
}

void sampler_destroy(struct Sampler *sampler) {
	//g_object_unref (sampler->pixbuf);
	delete sampler;
}


void sampler_set_falloff(struct Sampler *sampler, enum SamplerFalloff falloff) {
	sampler->falloff = falloff;
	switch (falloff){
	case NONE:
		sampler->falloff_fnc = sampler_falloff_none;
		break;
	case LINEAR:
		sampler->falloff_fnc = sampler_falloff_linear;
		break;
	case QUADRATIC:
		sampler->falloff_fnc = sampler_falloff_quadratic;
		break;
	case CUBIC:
		sampler->falloff_fnc = sampler_falloff_cubic;
		break;
	case EXPONENTIAL:
		sampler->falloff_fnc = sampler_falloff_exponential;
		break;
	default:
		sampler->falloff_fnc = 0;
	}
}

void sampler_set_oversample(struct Sampler *sampler, int oversample){
	sampler->oversample = oversample;
}

static void get_pixel(const guchar *pixels, int row_stride, int x, int y, Color* color)
{
	const guchar *p;
	p = pixels + y * row_stride + x * 3;
	color->rgb.red = p[0] / 255.0;
	color->rgb.green = p[1] / 255.0;
	color->rgb.blue = p[2] / 255.0;
}
int sampler_get_color_sample(struct Sampler *sampler, Vec2<int>& pointer, Rect2<int>& screen_rect, Vec2<int>& offset, Color* color)
{
	Color sample;
	Color result;
	float divider = 0;
	color_zero(&result);
	GdkPixbuf* pixbuf = screen_reader_get_pixbuf(sampler->screen_reader);
	int x = pointer.x, y = pointer.y;
	int left, right, top, bottom;
	left = max_int(screen_rect.getLeft(), x - sampler->oversample);
	right = min_int(screen_rect.getRight(), x + sampler->oversample + 1);
	top = max_int(screen_rect.getTop(), y - sampler->oversample);
	bottom = min_int(screen_rect.getBottom(), y + sampler->oversample + 1);
	int width = right - left;
	int height = bottom - top;
	int center_x = x - left;
	int center_y = y - top;
	int row_stride = gdk_pixbuf_get_rowstride(pixbuf);
	const guchar *pixels = gdk_pixbuf_read_pixels(pixbuf);
	float max_distance = 1 / sqrt(2 * pow((double)sampler->oversample, 2));
	for (int x=-sampler->oversample; x <= sampler->oversample; ++x){
		for (int y=-sampler->oversample; y <= sampler->oversample; ++y){
			if ((center_x + x < 0) || (center_y + y < 0)) continue;
			if ((center_x + x >= width) || (center_y + y >= height)) continue;
			get_pixel(pixels, row_stride, offset.x + center_x+x, offset.y + center_y+y, &sample);
			float f;
			if (sampler->oversample){
				f = sampler->falloff_fnc(sqrt((double)(x * x + y * y)) * max_distance);
			}else{
				f = 1;
			}
			color_multiply(&sample, f);
			color_add(&result, &sample);
			divider+=f;
		}
	}
	if (divider > 0)
		color_multiply(&result, 1 / divider);
	color_copy(&result, color);
	return 0;
}

enum SamplerFalloff sampler_get_falloff(struct Sampler *sampler){
	return sampler->falloff;
}

int sampler_get_oversample(struct Sampler *sampler) {
	return sampler->oversample;
}

void sampler_get_screen_rect(struct Sampler *sampler, math::Vec2<int>& pointer, math::Rect2<int>& screen_rect, math::Rect2<int> *rect)
{
	int left, right, top, bottom;
	left = max_int(screen_rect.getLeft(), pointer.x - sampler->oversample);
	right = min_int(screen_rect.getRight(), pointer.x + sampler->oversample + 1);
	top = max_int(screen_rect.getTop(), pointer.y - sampler->oversample);
	bottom = min_int(screen_rect.getBottom(), pointer.y + sampler->oversample + 1);
	*rect = math::Rect2<int>(left, top, right, bottom);
}

