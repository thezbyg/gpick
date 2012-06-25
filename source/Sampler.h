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


#ifndef SAMPLER_H_
#define SAMPLER_H_

#include "Color.h"
#include "Rect2.h"
#include "Vector2.h"
#include "ScreenReader.h"

enum SamplerFalloff{
	NONE = 0,
	LINEAR = 1,
	QUADRATIC = 2,
	CUBIC = 3,
	EXPONENTIAL = 4,
};

struct Sampler;

struct Sampler* sampler_new(struct ScreenReader* screen_reader);
void sampler_set_falloff(struct Sampler *sampler, enum SamplerFalloff falloff);
void sampler_set_oversample(struct Sampler *sampler, int oversample);

enum SamplerFalloff sampler_get_falloff(struct Sampler *sampler);
int sampler_get_oversample(struct Sampler *sampler);

void sampler_destroy(struct Sampler *sampler);

int sampler_get_color_sample(struct Sampler *sampler, math::Vec2<int>& pointer, math::Vec2<int>& screen_size, math::Vec2<int>& offset, Color* color);
void sampler_get_screen_rect(struct Sampler *sampler, math::Vec2<int>& pointer, math::Vec2<int>& screen_size, math::Rect2<int> *rect);

#endif /* SAMPLER_H_ */
