/*
 * Copyright (c) 2009-2021, Albertas Vyšniauskas
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

#ifndef GPICK_LAYOUT_STYLE_H_
#define GPICK_LAYOUT_STYLE_H_
#include "Color.h"
#include "Box.h"
#include "common/Ref.h"
#include "math/Vector.h"
#include <string>
namespace layout {
struct Box;
struct Style: public common::Ref<Style>::Counter {
	enum struct Type {
		unknown = 0,
		color,
		background,
		border,
	};
	Style(std::string_view name, Color color, float fontSize);
	virtual ~Style();
	const std::string &name() const;
	const std::string &label() const;
	Type type() const;
	Style &setLabel(std::string_view label);
	Color color() const;
	Style &setColor(Color color);
	bool dirty() const;
	Style &setDirty(bool dirty);
	float fontSize() const;
	math::Vector2f textOffset() const;
	Style &setTextOffset(math::Vector2f textOffset);
private:
	std::string m_name, m_label;
	Color m_color;
	float m_fontSize;
	Type m_type;
	bool m_dirty;
	math::Vector2f m_textOffset;
};
}
#endif /* LAYOUT_STYLE_H_ */
