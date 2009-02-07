/*
 * Copyright (c) 2009, Albertas Vyšniauskas
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

#include "MathUtil.h"
#include <math.h>

float
max_float_3(float a, float b, float c)
{
	if (a>b){
		if (a>c){
			return a;
		}else{
			return c;
		}
	}else{
		if (b>c){
			return b;
		}else{
			return c;
		}
	}
}

float
min_float_3(float a, float b, float c)
{
	if (a<b){
		if (a<c){
			return a;
		}else{
			return c;
		}
	}else{
		if (b<c){
			return b;
		}else{
			return c;
		}
	}
}

float
clamp_float( float x, float a, float b){
	if (x<a) return a;
	if (x>b) return b;
	return x;
}


int
clamp_int( int x, int a, int b){
	if (x<a) return a;
	if (x>b) return b;
	return x;
}

int
min_int( int a, int b)
{
	if (a>b) return b;
	return a;
}

int
max_int( int a, int b)
{
	if (a<b) return b;
	return a;
}

int
abs_int(int a)
{
	if (a<0) return -a;
	return a;
}

float
abs_float(float a)
{
	if (a<0) return -a;
	return a;
}

void
matrix3x3_identity(matrix3x3* matrix)
{
	matrix->m[0][0]=1;
	matrix->m[0][1]=0;
	matrix->m[0][2]=0;
	matrix->m[1][0]=0;
	matrix->m[1][1]=1;
	matrix->m[1][2]=0;
	matrix->m[2][0]=0;
	matrix->m[2][1]=0;
	matrix->m[2][2]=1;
}

void
vector2_set(vector2* v1, float x, float y)
{
	v1->x=x;
	v1->y=y;
}


float
vector2_length(vector2* v1)
{
	return sqrt(v1->x*v1->x+v1->y*v1->y);
}

void
vector2_normalize(vector2* v1, vector2* r)
{
	float l=vector2_length(v1);
	r->x=v1->x/l;
	r->y=v1->y/l;
}

float
vector2_dot(vector2* v1, vector2* v2)
{
	return v1->x*v2->x + v1->y*v2->y;
}

float mix_float( float a, float b, float mix){
	return a*(1-mix)+b*mix;
}
