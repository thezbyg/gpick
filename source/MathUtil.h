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

#ifndef MATHUTIL_H_
#define MATHUTIL_H_

#define PI 3.14159265

#define MIX(r,a,b,component) r.component = mix_float(a.component, b.component, step_i/(float)(steps-1))
#define MIX_COMPONENTS(r,a,b,ca,cb,cc) MIX(r,a,b,ca); MIX(r,a,b,cb); MIX(r,a,b,cc)

float min_float_3(float a, float b, float c);

float max_float_3(float a, float b, float c);

int min_int(int a, int b);

int max_int(int a, int b);

int wrap_int(int x, int a, int b);

float clamp_float(float x, float a, float b);

float wrap_float(float x);

float mix_float(float a, float b, float mix);

int clamp_int(int x, int a, int b);

int abs_int(int a);

float abs_float(float a);

double mix_double(double a, double b, double mix);


typedef struct matrix3x3{
	double m[3][3];
}matrix3x3;

void matrix3x3_identity(matrix3x3* matrix);
void matrix3x3_multiply(const matrix3x3* matrix1, const matrix3x3* matrix2, matrix3x3* result);
double matrix3x3_determinant(const matrix3x3* matrix);
int matrix3x3_inverse(const matrix3x3* matrix, matrix3x3* result);
void matrix3x3_transpose(const matrix3x3* matrix, matrix3x3* result);

typedef struct vector2{
	float x;
	float y;
}vector2;

void vector2_set(vector2* v1, float x, float y);

float vector2_length(const vector2* v1);

void vector2_normalize(const vector2* v1, vector2* r);

float vector2_dot(const vector2* v1, const vector2* v2);


typedef struct vector3{
	union{
		struct{
			float x;
			float y;
			float z;
		};
		float m[3];
	};
}vector3;

void vector3_set(vector3* vector, float x, float y, float z);
void vector3_copy(const vector3* vector, vector3* result);

float vector3_length(const vector3* vector);

void vector3_multiply_matrix3x3(const vector3* vector, const matrix3x3* matrix, vector3* result );

void vector3_clamp(vector3* vector, float a, float b);

#endif /* MATHUTIL_H_ */

