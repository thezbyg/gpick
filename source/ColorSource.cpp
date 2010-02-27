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

#include "ColorSource.h"

#include <iostream>
using namespace std;

int color_source_init(ColorSource* source){
	source->set_color = 0;
	source->get_color = 0;
	source->activate = 0;
	source->deactivate = 0;
	source->destroy = 0;
	source->userdata = 0;
	return 0;
}

int color_source_activate(ColorSource *source){
	if (source->activate) return source->activate(source);
	return -1;
}

int color_source_deactivate(ColorSource *source){
	if (source->deactivate) return source->deactivate(source);
	return -1;
}

int color_source_set_color(ColorSource *source, ColorObject *color){
	if (source && source->set_color)
		return source->set_color(source, color);
	else if (!source)
		cerr << "Color source undefined" << endl;
	return -1;
}

int color_source_get_color(ColorSource *source, ColorObject **color){
	if (source && source->get_color)
		return source->get_color(source, color);
	else if (!source)
		cerr << "Color source undefined" << endl;
	return -1;
}

int color_source_destroy(ColorSource* source){
	if (source->destroy) return source->destroy(source);
	delete source;
	return 0;
}
