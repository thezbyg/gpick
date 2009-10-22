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
 

#include "ColorRYB.h"
#include "MathUtil.h"

#include "BezierCubicCurve.h"
#include "Vector2.h"

#include <math.h>
#include <stdio.h>
#include <list>
using namespace std;

typedef math::Vec2<double> point;
typedef math::BezierCubicCurve<point, double> bezier;

double bezier_eval_at_x(list<bezier*>& channel, double x, double delta){
	for (list<bezier*>::iterator i=channel.begin(); i!=channel.end(); ++i){
		if (x>=(*i)->p0.x && x<=(*i)->p3.x){
			double width = ((*i)->p3.x-(*i)->p0.x);
			double t = (x-(*i)->p0.x)/width;
			double d;
			point v;


			for (int limit=50; limit>0; --limit){
				v = (**i)(t);
				d = v.x - x;
				
				if (fabs(d)<delta) return v.y;
				
				//printf("%f %f %f %f\n", t, v.x, v.y, d);
				
				t -= d/width;
				if (t>1)
					t=1;
				else if (t<0)
					t=0;
			}
		}
	}
	return 0;
}

static void color_get_ryb_curves(list<bezier*> &red, list<bezier*> &green, list<bezier*> &blue){
	static bezier red_v[]={
		bezier(	point(0.0, 1.0), point(1.0, 1.0), point(13.0, 1.0), point(14.0, 1.0) ),
		bezier(	point(14.0, 1.0), point(16.0, 0.6405), point(16.9, 0.0), point(21.0, 0.0) ),
		bezier(	point(21.0, 0.0), point(28.0, 0.0), point(33.0, 1.0), point(36.0, 1.0) )
	};
	static bezier green_v[]={
		bezier(	point(0.0, 0.0), point(4.0, 0.4), point(13.0, 1.0), point(14.0, 1.0) ),
		bezier(	point(14.0, 1.0), point(14.85, 1.0), point(17.05, 0.9525), point(19.0, 0.7) ),
		bezier(	point(19.0, 0.7), point(24.0, 0.05), point(31.0, 0.0), point(36.0, 0.0) )
	};
	static bezier blue_v[]={
		bezier(	point(0.0, 0.0), point(1.0, 0.0), point(18.0, 0.0), point(19.0, 0.0) ),
		bezier(	point(19.0, 0.0), point(22.0, 0.9), point(33.0, 0.9), point(36.0, 0.0) )
	};
	for (int i=0; i<sizeof(red_v)/sizeof(bezier); ++i){
		red.push_back(&red_v[i]);
	}
	for (int i=0; i<sizeof(green_v)/sizeof(bezier); ++i){
		green.push_back(&green_v[i]);
	}
	for (int i=0; i<sizeof(blue_v)/sizeof(bezier); ++i){
		blue.push_back(&blue_v[i]);
	}
	/*red.push_back(
		new bezier(	point(0.0, 1.0), point(1.0, 1.0), point(13.0, 1.0), point(14.0, 1.0) )
	);
	red.push_back(
		new bezier(	point(14.0, 1.0), point(16.0, 0.6405), point(16.9, 0.0), point(21.0, 0.0) )
	);
	red.push_back(
		new bezier(	point(21.0, 0.0), point(28.0, 0.0), point(33.0, 1.0), point(36.0, 1.0) )
	);
	
	green.push_back(
		new bezier(	point(0.0, 0.0), point(4.0, 0.4), point(13.0, 1.0), point(14.0, 1.0) )
	);
	green.push_back(
		new bezier(	point(14.0, 1.0), point(14.85, 1.0), point(17.05, 0.9525), point(19.0, 0.7) )
	);
	green.push_back(
		new bezier(	point(19.0, 0.7), point(24.0, 0.05), point(31.0, 0.0), point(36.0, 0.0) )
	);
	
	blue.push_back(
		new bezier(	point(0.0, 0.0), point(1.0, 0.0), point(18.0, 0.0), point(19.0, 0.0) )
	);
	blue.push_back(
		new bezier(	point(19.0, 0.0), point(22.0, 0.9), point(33.0, 0.9), point(36.0, 0.0) )
	);*/
}

int color_rgbhue_to_rybhue(double rgb_hue, double* ryb_hue){
	list<bezier*> red, green, blue;
	color_get_ryb_curves(red, green, blue);
	
	double hue = rgb_hue;
	double d;
	
	double delta = 1/3600.0;
	
	Color color, color2;
	for (int limit=100; limit>0; --limit){
		color.rgb.red = bezier_eval_at_x(red, hue*36, 0.01),
		color.rgb.green = bezier_eval_at_x(green, hue*36, 0.01),
		color.rgb.blue = bezier_eval_at_x(blue, hue*36, 0.01);
	
		color_rgb_to_hsv(&color, &color2);
		
		d = rgb_hue - color2.hsv.hue;
		if (fabs(d)<delta){
			*ryb_hue = hue;
			return 0;
		}
		
		hue += d/2;
		if (hue>1) hue=1;
		else if (hue<0) hue=0;		
	}
	*ryb_hue = hue;
	return -1;
}

void color_rybhue_to_rgb(double hue, Color* color){
	list<bezier*> red, green, blue;
	color_get_ryb_curves(red, green, blue);
	
	color->rgb.red = bezier_eval_at_x(red, hue*36, 0.01),
	color->rgb.green = bezier_eval_at_x(green, hue*36, 0.01),
	color->rgb.blue = bezier_eval_at_x(blue, hue*36, 0.01);
}

double color_ryb_transform_lightness(double hue1, double hue2){

	double t;
	hue1 = modf(hue1, &t);
	hue2 = modf(hue2, &t);
	
	double values[]={
		0.50000000,		0.50000000,		0.50000000,		0.50000000,
		0.50000000,		0.50000000,		0.50000000,		0.50000000,
		0.50000000,		0.50000000,		0.50000000,		0.50000000,
		0.50000000,		0.46470600,		0.42745101,		0.39215699,
		0.37450999,		0.35490200,		0.33725500,		0.35294101,
		0.36862749,		0.38431349,		0.37254900,		0.36078450,
		0.34901950,		0.34313750,		0.33529401,		0.32941201,
		0.32549000,		0.31960800,		0.31568649,		0.33921549,
		0.36470601,		0.38823551,		0.42548999,		0.46274501,
		0.50000000,
	};
	int32_t samples=sizeof(values)/sizeof(double)-1;
	double n;
	return 	mix_double(values[int(floor(hue2*samples))], values[int(floor(hue2*samples))+1], modf(hue2*samples,&n))/
			mix_double(values[int(floor(hue2*samples))], values[int(floor(hue1*samples))+1], modf(hue1*samples,&n));
}

double color_ryb_transform_hue(double hue, bool forward){
	double values[]={
		0.00000000,		0.02156867,		0.04248367,		0.06405234,
		0.07385617,		0.08431367,		0.09411767,		0.10653600,
		0.11830067,		0.13071899,		0.14248367,		0.15490200,
		0.16666667,		0.18354435,		0.19954120,		0.21666662,
		0.25130904,		0.28545108,		0.31976745,		0.38981494,
		0.46010628,		0.53061217,		0.54649121,		0.56159425,
		0.57771534,		0.60190469,		0.62573093,		0.64980155,
		0.68875504,		0.72801632,		0.76708061,		0.80924863,
		0.85215056,		0.89478123,		0.92933953,		0.96468931,
		1.00000000,		1.00000000,
	};

	int32_t samples=sizeof(values)/sizeof(double)-2;
	double new_hue;
	
	double t;
	hue = modf(hue, &t);

	if (!forward){
		for (uint32_t i=0; i<samples; ++i){
			if (values[i+1]>=hue){
				int index1, index2;
				double value1, value2, mix;

				index1=i;
				index2=i+1;

				value1=index1 / (double)samples;
				value2=index2 / (double)samples;

				mix = (hue-values[index1])/(values[index2]-values[index1]);

				new_hue= mix_double(value1, value2, mix);

				return new_hue;
			}
		}
		return 1;
	}else{
		double value1=values[int(hue*samples)];
		double value2=values[int(hue*samples+1)];

		double n;
		double mix = modf(hue*samples, &n);

		new_hue = mix_double(value1, value2, mix);

		return new_hue;
	}
	return 0;

}

