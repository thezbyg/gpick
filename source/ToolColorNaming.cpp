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

ToolColorNameAssigner::ToolColorNameAssigner(GlobalState *gs):m_gs(gs){
	m_color_naming_type = tool_color_naming_name_to_type(dynv_get_string_wd(m_gs->params, "gpick.color_names.tool_color_naming", "tool_specific"));
	if (m_color_naming_type == TOOL_COLOR_NAMING_AUTOMATIC_NAME){
		m_imprecision_postfix = dynv_get_bool_wd(m_gs->params, "gpick.color_names.imprecision_postfix", true);
	}else{
		m_imprecision_postfix = false;
	}
}

ToolColorNameAssigner::~ToolColorNameAssigner(){
}

void ToolColorNameAssigner::assign(struct ColorObject *color_object, Color *color){
	string name;
	switch (m_color_naming_type){
		case TOOL_COLOR_NAMING_UNKNOWN:
		case TOOL_COLOR_NAMING_EMPTY:
			dynv_set_string(color_object->params, "name", "");
			break;
		case TOOL_COLOR_NAMING_AUTOMATIC_NAME:
			name = color_names_get(m_gs->color_names, color, m_imprecision_postfix);
			dynv_set_string(color_object->params, "name", name.c_str());
			break;
		case TOOL_COLOR_NAMING_TOOL_SPECIFIC:
			name = getToolSpecificName(color_object, color);
			dynv_set_string(color_object->params, "name", name.c_str());
			break;
	}
}

std::string ToolColorNameAssigner::getToolSpecificName(struct ColorObject *color_object, Color *color){
	return string("");
}

