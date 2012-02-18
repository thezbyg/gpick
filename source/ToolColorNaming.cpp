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

#include "ToolColorNaming.h"
#include "Internationalisation.h"
#include <string>
using namespace std;

static ToolColorNamingOption options[] = {
	{TOOL_COLOR_NAMING_EMPTY, "empty", 0},
	{TOOL_COLOR_NAMING_AUTOMATIC_NAME, "automatic_name", 0},
	{TOOL_COLOR_NAMING_TOOL_SPECIFIC, "tool_specific", 0},
	{TOOL_COLOR_NAMING_UNKNOWN, 0, 0},
};

const ToolColorNamingOption* tool_color_naming_get_options(){
  options[0].label = _("_Empty");
  options[1].label = _("_Automatic name");
  options[2].label = _("_Tool specific");
	return options;
}

ToolColorNamingType tool_color_naming_name_to_type(const char *name){
	string n = name;
	int i = 0;
	while (options[i].name){
		if (n.compare(options[i].name) == 0){
			return options[i].type;
		}
		i++;
	}
	return TOOL_COLOR_NAMING_UNKNOWN;
}

