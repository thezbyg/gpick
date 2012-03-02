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

#ifndef TOOL_COLOR_NAMING_H_
#define TOOL_COLOR_NAMING_H_

#include "GlobalStateStruct.h"
#include "DynvHelpers.h"

enum ToolColorNamingType {
	TOOL_COLOR_NAMING_UNKNOWN = 0,
	TOOL_COLOR_NAMING_EMPTY,
	TOOL_COLOR_NAMING_AUTOMATIC_NAME,
	TOOL_COLOR_NAMING_TOOL_SPECIFIC,
};

typedef struct ToolColorNamingOption{
	ToolColorNamingType type;
	const char *name;
	const char *label;
}ToolColorNamingOption;

const ToolColorNamingOption* tool_color_naming_get_options();
ToolColorNamingType tool_color_naming_name_to_type(const char *name);

class ToolColorNameAssigner{
	protected:
		ToolColorNamingType m_color_naming_type;
		GlobalState* m_gs;
		bool m_imprecision_postfix;
	public:
		ToolColorNameAssigner(GlobalState *gs);
		virtual ~ToolColorNameAssigner();

		void assign(struct ColorObject *color_object, Color *color);

		virtual std::string getToolSpecificName(struct ColorObject *color_object, Color *color);
};

#endif /* TOOL_COLOR_NAMING_H_ */

