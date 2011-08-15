/*
 * Copyright (c) 2009-2010, Albertas Vyšniauskas
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
#include <string.h>

float max_float_3(float a, float b, float c) {
	if (a > b){
		if (a > c){
			return a;
		}else{
			return c;
		}
	}else{
		if (b > c){
			return b;
		}else{
			return c;
		}
	}
}

float min_float_3(float a, float b, float c) {
	if (a < b){
		if (a < c){
			return a;
		}else{
			return c;
		}
	}else{
		if (b < c){
			return b;
		}else{
			return c;
		}
	}
}

float clamp_float(float x, float a, float b) {
	if (x < a)
		return a;
	if (x > b)
		return b;
	return x;
}


int clamp_int(int x, int a, int b) {
	if (x < a)
		return a;
	if (x > b)
		return b;
	return x;
}

int min_int(int a, int b) {
	if (a > b)
		return b;
	return a;
}

int max_int(int a, int b) {
	if (a < b)
		return b;
	return a;
}

int abs_int(int a) {
	if (a < 0)
		return -a;
	return a;
}

float abs_float(float a) {
	if (a < 0)
		return -a;
	return a;
}

void matrix3x3_identity(matrix3x3* matrix) {
	int i,j;
	for (i=0;i<3;++i){
		for (j=0;j<3;++j){
			matrix->m[i][j]=((i==j)?1:0);
		}
	}
}

void matrix3x3_copy(matrix3x3* matrix, matrix3x3* result) {
	memcpy(result, matrix, sizeof(matrix3x3));
}

void matrix3x3_multiply(matrix3x3* matrix1, matrix3x3* matrix2, matrix3x3* result){
	int i,j,k;

	matrix3x3 matrix_t;

	if (matrix1==result){
		matrix3x3_copy(matrix1, &matrix_t);
		matrix1=&matrix_t;
	}else if (matrix2==result){
		matrix3x3_copy(matrix2, &matrix_t);
		matrix2=&matrix_t;
	}

	for (i=0;i<3;++i){
		for (j=0;j<3;++j){
			result->m[i][j]=0;
			for (k=0;k<3;++k){
				result->m[i][j] += matrix1->m[k][j]*matrix2->m[i][k];
			}
		}
	}
}

double matrix3x3_determinant(matrix3x3* matrix) {
	double det=0;
	double t;
	int i,j;
	for (i=0;i<3;++i){
		t=1;
		for (j=0;j<3;++j){
			t*=matrix->m[(i+j)%3][j];
		}
		det+=t;
	}
	for (i=0;i<3;++i){
		t=1;
		for (j=0;j<3;++j){
			t*=matrix->m[(i+2-j)%3][j];
		}
		det-=t;
	}
	return det;
}

void matrix3x3_transpose(matrix3x3* matrix, matrix3x3* result) {
	int i,j;
	matrix3x3 matrix_t;

	if (matrix==result){
		matrix3x3_copy(matrix, &matrix_t);
		matrix=&matrix_t;
	}

	for (i=0;i<3;++i){
		for (j=0;j<3;++j){
			result->m[j][i]=matrix->m[i][j];
		}
	}

}

int matrix3x3_inverse(matrix3x3* matrix, matrix3x3* result){
	double det=matrix3x3_determinant(matrix);
	if (det==0) return -1;

	double invdet=1/det;

	matrix3x3 matrix_t;

	if (matrix==result){
		matrix3x3_copy(matrix, &matrix_t);
		matrix=&matrix_t;
	}

	result->m[0][0] = (matrix->m[1][1] * matrix->m[2][2] - matrix->m[2][1] * matrix->m[1][2]) * invdet;
	result->m[0][1] = -(matrix->m[0][1] * matrix->m[2][2] - matrix->m[2][1] * matrix->m[0][2]) * invdet;
	result->m[0][2] = (matrix->m[0][1] * matrix->m[1][2] - matrix->m[1][1] * matrix->m[0][2]) * invdet;

	result->m[1][0] = -(matrix->m[1][0] * matrix->m[2][2] - matrix->m[2][0] * matrix->m[1][2]) * invdet;
	result->m[1][1] = (matrix->m[0][0] * matrix->m[2][2] - matrix->m[2][0] * matrix->m[0][2]) * invdet;
	result->m[1][2] = -(matrix->m[0][0] * matrix->m[1][2] - matrix->m[1][0] * matrix->m[0][2]) * invdet;

	result->m[2][0] = (matrix->m[1][0] * matrix->m[2][1] - matrix->m[2][0] * matrix->m[1][1]) * invdet;
	result->m[2][1] = -(matrix->m[0][0] * matrix->m[2][1] - matrix->m[2][0] * matrix->m[0][1]) * invdet;
	result->m[2][2] = (matrix->m[0][0] * matrix->m[1][1] - matrix->m[1][0] * matrix->m[0][1]) * invdet;

	return 0;
}

void vector2_set(vector2* v1, float x, float y) {
	v1->x = x;
	v1->y = y;
}


float vector2_length(vector2* v1) {
	return sqrt(v1->x * v1->x + v1->y * v1->y);
}

void vector2_normalize(vector2* v1, vector2* r) {
	float l = vector2_length(v1);
	r->x = v1->x / l;
	r->y = v1->y / l;
}

float vector2_dot(vector2* v1, vector2* v2) {
	return v1->x * v2->x + v1->y * v2->y;
}

float mix_float(float a, float b, float mix) {
	return a * (1 - mix) + b * mix;
}

double mix_double(double a, double b, double mix){
	return a * (1 - mix) + b * mix;
}

float wrap_float(float x) {
	return x - floor(x);
}

int wrap_int(int x, int a, int b) {
	if (x < a){
		return b - (a - x) % (b - a);
	}else if (x >= b){
		return a + (x - b) % (b - a);
	}
	return x;
}


void vector3_multiply_matrix3x3(vector3* vector, matrix3x3* matrix, vector3* result) {

	vector3 vector_t;

	if (vector==result){
		vector3_copy(vector, &vector_t);
		vector=&vector_t;
	}

	result->x = vector->x * matrix->m[0][0] + vector->y * matrix->m[0][1] + vector->z * matrix->m[0][2];
	result->y = vector->x * matrix->m[1][0] + vector->y * matrix->m[1][1] + vector->z * matrix->m[1][2];
	result->z = vector->x * matrix->m[2][0] + vector->y * matrix->m[2][1] + vector->z * matrix->m[2][2];
}

void vector3_set(vector3* vector, float x, float y, float z) {
	vector->x = x;
	vector->y = y;
	vector->z = z;
}

void vector3_copy(vector3* vector, vector3* result){
	memcpy(result, vector, sizeof(vector3));
}

float vector3_length(vector3* vector) {
	return sqrt(vector->x * vector->x + vector->y * vector->y + vector->z * vector->z);
}

void vector3_clamp(vector3* vector, float a, float b){
	vector->x = clamp_float(vector->x, a, b);
	vector->y = clamp_float(vector->y, a, b);
	vector->z = clamp_float(vector->z, a, b);
}

