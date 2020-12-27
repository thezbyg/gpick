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

#include "Sampler.h"
#include "ScreenReader.h"
#include "MathUtil.h"
#include <cmath>
#include <gdk/gdk.h>
using namespace math;

struct Sampler
{
	int oversample;
	SamplerFalloff falloff;
	float (*falloff_fnc)(float distance);
	ScreenReader* screen_reader;
};
static float sampler_falloff_none(float distance)
{
	return 1;
}
static float sampler_falloff_linear(float distance)
{
	return 1 - distance;
}
static float sampler_falloff_quadratic(float distance)
{
	return 1 - (distance * distance);
}
static float sampler_falloff_cubic(float distance)
{
	return 1 - (distance * distance * distance);
}
static float sampler_falloff_exponential(float distance)
{
	return 1 / exp(5 * distance * distance);
}
struct Sampler* sampler_new(ScreenReader* screen_reader)
{
	Sampler* sampler = new Sampler;
	sampler->oversample = 0;
	sampler_set_falloff(sampler, SamplerFalloff::none);
	sampler->screen_reader = screen_reader;
	return sampler;
}
void sampler_destroy(Sampler *sampler)
{
	delete sampler;
}
void sampler_set_falloff(Sampler *sampler, SamplerFalloff falloff)
{
	sampler->falloff = falloff;
	switch (falloff){
	case SamplerFalloff::none:
		sampler->falloff_fnc = sampler_falloff_none;
		break;
	case SamplerFalloff::linear:
		sampler->falloff_fnc = sampler_falloff_linear;
		break;
	case SamplerFalloff::quadratic:
		sampler->falloff_fnc = sampler_falloff_quadratic;
		break;
	case SamplerFalloff::cubic:
		sampler->falloff_fnc = sampler_falloff_cubic;
		break;
	case SamplerFalloff::exponential:
		sampler->falloff_fnc = sampler_falloff_exponential;
		break;
	default:
		sampler->falloff_fnc = 0;
	}
}
void sampler_set_oversample(Sampler *sampler, int oversample)
{
	sampler->oversample = oversample;
}
static void get_pixel(unsigned char *data, int stride, int x, int y, Color* color)
{
	unsigned char *p;
	p = data + y * stride + x * 4;
	color->rgb.red = p[2] * (1 / 255.0f);
	color->rgb.green = p[1] * (1 / 255.0f);
	color->rgb.blue = p[0] * (1 / 255.0f);
}
int sampler_get_color_sample(Sampler *sampler, Vec2<int>& pointer, Rect2<int>& screen_rect, Vec2<int>& offset, Color* color)
{
	Color sample, result = { 0.0f };
	float divider = 0;
	cairo_surface_t *surface = screen_reader_get_surface(sampler->screen_reader);
	int x = pointer.x, y = pointer.y;
	int left, right, top, bottom;
	left = math::max(screen_rect.getLeft(), x - sampler->oversample);
	right = math::min(screen_rect.getRight(), x + sampler->oversample + 1);
	top = math::max(screen_rect.getTop(), y - sampler->oversample);
	bottom = math::min(screen_rect.getBottom(), y + sampler->oversample + 1);
	int width = right - left;
	int height = bottom - top;
	int center_x = x - left;
	int center_y = y - top;
	float max_distance = static_cast<float>(1 / std::sqrt(2 * std::pow((double)sampler->oversample, 2)));
	unsigned char *data = cairo_image_surface_get_data(surface);
	int stride = cairo_image_surface_get_stride(surface);
	for (int x=-sampler->oversample; x <= sampler->oversample; ++x){
		for (int y=-sampler->oversample; y <= sampler->oversample; ++y){
			if ((center_x + x < 0) || (center_y + y < 0)) continue;
			if ((center_x + x >= width) || (center_y + y >= height)) continue;
			get_pixel(data, stride, offset.x + center_x + x, offset.y + center_y + y, &sample);
			float f;
			if (sampler->oversample){
				f = sampler->falloff_fnc(static_cast<float>(std::sqrt((double)(x * x + y * y)) * max_distance));
			}else{
				f = 1;
			}
			result += sample * f;
			divider += f;
		}
	}
	if (divider > 0)
		result *= 1 / divider;
	*color = result;
	return 0;
}
SamplerFalloff sampler_get_falloff(Sampler *sampler)
{
	return sampler->falloff;
}
int sampler_get_oversample(Sampler *sampler)
{
	return sampler->oversample;
}
void sampler_get_screen_rect(Sampler *sampler, math::Vec2<int>& pointer, math::Rect2<int>& screen_rect, math::Rect2<int> *rect)
{
	int left, right, top, bottom;
	left = math::max(screen_rect.getLeft(), pointer.x - sampler->oversample);
	right = math::min(screen_rect.getRight(), pointer.x + sampler->oversample + 1);
	top = math::max(screen_rect.getTop(), pointer.y - sampler->oversample);
	bottom = math::min(screen_rect.getBottom(), pointer.y + sampler->oversample + 1);
	*rect = math::Rect2<int>(left, top, right, bottom);
}
