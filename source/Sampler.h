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

#ifndef GPICK_SAMPLER_H_
#define GPICK_SAMPLER_H_

#include "Color.h"
#include "Rect2.h"
#include "Vector2.h"
struct Sampler;
struct ScreenReader;
enum class SamplerFalloff: int
{
	none = 0,
	linear = 1,
	quadratic = 2,
	cubic = 3,
	exponential = 4,
};
Sampler* sampler_new(ScreenReader* screen_reader);
void sampler_set_falloff(Sampler *sampler, SamplerFalloff falloff);
void sampler_set_oversample(Sampler *sampler, int oversample);
SamplerFalloff sampler_get_falloff(Sampler *sampler);
int sampler_get_oversample(Sampler *sampler);
void sampler_destroy(Sampler *sampler);
int sampler_get_color_sample(Sampler *sampler, math::Vec2<int>& pointer, math::Rect2<int>& screen_rect, math::Vec2<int>& offset, Color* color);
void sampler_get_screen_rect(Sampler *sampler, math::Vec2<int>& pointer, math::Rect2<int>& screen_rect, math::Rect2<int> *rect);

#endif /* GPICK_SAMPLER_H_ */
