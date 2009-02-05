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

#include "ColorNames.h"
#include "MathUtil.h"

#include <math.h>
#include <sstream>
#include <fstream>
#include <iostream>
using namespace std;

ColorNames*
color_names_new()
{
	ColorNames* cnames = new ColorNames;
	return cnames;
}


void color_names_strip_spaces(string& string_x, string& stripchars){
   if(string_x.empty()) return;
   if (stripchars.empty()) return;

   int startIndex = string_x.find_first_not_of(stripchars);
   int endIndex = string_x.find_last_not_of(stripchars);

   if ((startIndex==string::npos)||(endIndex==string::npos)){
	   string_x.erase();
	   return;
   }
   string_x = string_x.substr(startIndex, (endIndex-startIndex)+1 );
}


int
color_names_load_from_file(ColorNames* cnames, const char* filename)
{
	ifstream file(filename, ifstream::in);

	if (file.is_open())
	{
		string line;
		stringstream rline (ios::in | ios::out);
		Color color;
		string name;
		string strip_chars = " \t,.\n\r";

		while (!(file.eof()))
		{
			getline(file, line);
			if (line.empty()) continue;

			if (line.at(0)=='!') continue;

			rline.clear();
			rline.str(line);


			rline>>color.rgb.red>>color.rgb.green>>color.rgb.blue;
			getline(rline, name);

			color_names_strip_spaces(name, strip_chars);
			color_multiply(&color, 1/255.0);

			ColorNameEntry* entry = new ColorNameEntry;
			entry->name=name;
			color_rgb_to_xyz(&color, &entry->color);

			cnames->color_list.push_back(entry);

			//cout<<color.rgb.red<<" "<<color.rgb.green<<" "<<color.rgb.blue<<" "<<name<<endl;

		}

		file.close();
		return 0;
	}
	return -1;
}

void
color_names_destroy(ColorNames* cnames)
{
	for (list<ColorNameEntry*>::iterator i=cnames->color_list.begin();i!=cnames->color_list.end();++i){
		delete (*i);
	}
	delete cnames;
}

string
color_names_get(ColorNames* cnames, Color* color)
{
	Color c1;
	color_rgb_to_xyz(color, &c1);

	float result_delta=1e5;
	list<ColorNameEntry*>::iterator result_i=cnames->color_list.end();

	for (list<ColorNameEntry*>::iterator i=cnames->color_list.begin();i!=cnames->color_list.end();++i){
		float delta = pow((*i)->color.xyz.x-c1.xyz.x,2) + pow((*i)->color.xyz.y-c1.xyz.y,2) + pow((*i)->color.xyz.z-c1.xyz.z,2);
		if (delta<result_delta)
		{
			result_delta=delta;
			result_i=i;
		}
	}

	if (result_i!=cnames->color_list.end()){
		stringstream s;
		s<<(*result_i)->name<<" "<<int((0.01-result_delta)*10000);
		return s.str();//(*result_i)->name;
	}
	return string("");
}
