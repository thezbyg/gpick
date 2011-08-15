/*
 * Copyright (c) 2009-2011, Albertas Vyšniauskas
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

#ifndef TRANSFORMATION_COLOR_VISION_DEFICIENCY_H_
#define TRANSFORMATION_COLOR_VISION_DEFICIENCY_H_

#include "Transformation.h"

namespace transformation {

class ColorVisionDeficiency;

class ColorVisionDeficiencyConfig: public Configuration{
	protected:
		GtkWidget *main;
		GtkWidget *type;
		GtkWidget *strength;
	public:
		ColorVisionDeficiencyConfig(ColorVisionDeficiency &transformation);
		virtual ~ColorVisionDeficiencyConfig();

		virtual GtkWidget* getWidget();
		virtual void applyConfig(dynvSystem *dynv);
};

class ColorVisionDeficiency: public Transformation{
	public:
		enum DeficiencyType{
			PROTANOMALY,
			DEUTERANOMALY,
			TRITANOMALY,
			PROTANOPIA,
			DEUTERANOPIA,
			TRITANOPIA,
			DEFICIENCY_TYPE_COUNT,
		};
		static const char *deficiency_type_string[];

		static const char *getName();
		static const char *getReadableName();
	protected:
		float strength;
		DeficiencyType type;
		virtual void apply(Color *input, Color *output);
	public:
		ColorVisionDeficiency();
		ColorVisionDeficiency(DeficiencyType type, float strength);
		virtual ~ColorVisionDeficiency();

		virtual void serialize(struct dynvSystem *dynv);
		virtual void deserialize(struct dynvSystem *dynv);

		virtual boost::shared_ptr<Configuration> getConfig();

		DeficiencyType typeFromString(const char *type_string);

	friend class ColorVisionDeficiencyConfig;
};

}

#endif /* TRANSFORMATION_COLOR_VISION_DEFICIENCY_H_ */


