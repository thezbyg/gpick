/*
 * Copyright (c) 2009-2016, Albertas Vyšniauskas
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

#ifndef GPICK_COLOR_OBJECT_H_
#define GPICK_COLOR_OBJECT_H_

class ColorObject;
#include "Color.h"
#include <string>

class ColorObject
{
	public:
		ColorObject();
		ColorObject(const char *name, const Color &color);
		ColorObject(const std::string &name, const Color &color);
		ColorObject *reference();
		void release();
		const Color &getColor() const;
		void setColor(const Color &color);
		const std::string &getName() const;
		void setName(const std::string &name);
		ColorObject* copy() const;
		bool isSelected() const;
		bool isVisited() const;
		size_t getPosition() const;
		bool isPositionSet() const;
		void setPosition(size_t position);
		void resetPosition();
		void setSelected(bool selected);
		void setVisited(bool visited);
		size_t getReferenceCount() const;
		void setVisible(bool visible);
		bool isVisible() const;
	private:
		size_t m_refcnt;
		std::string m_name;
		Color m_color;
		size_t m_position;
		bool m_position_set;
		bool m_selected;
		bool m_visited;
		bool m_visible;
};

#endif /* GPICK_COLOR_OBJECT_H_ */
