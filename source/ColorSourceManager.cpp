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

#include "ColorSourceManager.h"

#include <glib.h>
#include <iostream>
using namespace std;


ColorSourceManager* color_source_manager_create(){
	ColorSourceManager *csm = new ColorSourceManager;


	return csm;
}

int color_source_manager_add_source(ColorSourceManager *csm, ColorSource *source){
	pair<map<string, ColorSource*>::iterator, bool> r;
	r = csm->colorsource.insert(pair<string, ColorSource*>(source->identificator, source));
	return r.second;
}

ColorSource* color_source_manager_get(ColorSourceManager *csm, const char *name){
	map<string, ColorSource*>::iterator i = csm->colorsource.find(name);
	if (i != csm->colorsource.end()){
		return (*i).second;
	}
	return 0;
}

vector<ColorSource*> color_source_manager_get_all(ColorSourceManager *csm){
	vector<ColorSource*> ret;
	ret.resize(csm->colorsource.size());

	uint32_t j = 0;
	for (map<string, ColorSource*>::iterator i = csm->colorsource.begin(); i != csm->colorsource.end(); ++i){
		ret[j] = (*i).second;
		j++;
	}
	return ret;
}

int color_source_manager_destroy(ColorSourceManager *csm){
	for (map<string, ColorSource*>::iterator i = csm->colorsource.begin(); i != csm->colorsource.end(); ++i){
		color_source_destroy((*i).second);
	}
	csm->colorsource.clear();
	delete csm;
	return 0;
}





