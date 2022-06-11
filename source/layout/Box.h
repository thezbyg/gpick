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

#ifndef GPICK_LAYOUT_BOX_H_
#define GPICK_LAYOUT_BOX_H_
#include "common/Ref.h"
#include "math/Rectangle.h"
#include "math/Vector.h"
#include <string>
#include <string_view>
#include <vector>
namespace layout {
struct Context;
struct Style;
struct Box: public common::Ref<Box>::Counter {
	Box(std::string_view name, float x, float y, float width, float height);
	virtual ~Box();
	virtual void draw(Context &context, const math::Rectanglef &parentRect);
	void drawChildren(Context &context, const math::Rectanglef &parentRect);
	void addChild(common::Ref<Box> box);
	Box &setStyle(common::Ref<Style> style);
	Box &setLocked(bool locked);
	Box &setHelperOnly(bool helperOnly);
	const std::string &name();
	common::Ref<Style> style();
	const math::Rectanglef &rect() const;
	common::Ref<Box> getBoxAt(const math::Vector2f &point);
	common::Ref<Box> getNamedBox(std::string_view name);
	bool locked() const;
	bool helperOnly() const;
	virtual bool hitTest(const math::Vector2f &point) const;
	template<typename Callable>
	void visit(const math::Rectanglef &rectangle, Callable &&callable) {
		math::Rectanglef thisRectangle = m_rect.impose(rectangle);
		callable(thisRectangle, *this);
		for (auto &child: m_children) {
			child->visit(thisRectangle, callable);
		}
	}
private:
	std::string m_name;
	common::Ref<Style> m_style;
	bool m_helperOnly, m_locked;
	math::Rectanglef m_rect;
	std::vector<common::Ref<Box>> m_children;
};
struct Text: public Box {
	Text(std::string_view name, float x, float y, float width, float height);
	virtual ~Text();
	virtual void draw(Context &context, const math::Rectanglef &parentRect) override;
	Text &setText(std::string_view text);
	Text &setStyle(common::Ref<Style> style);
	Text &setLocked(bool locked);
	Text &setHelperOnly(bool helperOnly);
	Text *reference();
private:
	std::string m_text;
};
struct Fill: public Box {
	Fill(std::string_view name, float x, float y, float width, float height);
	virtual ~Fill();
	virtual void draw(Context &context, const math::Rectanglef &parentRect) override;
	Fill &setStyle(common::Ref<Style> style);
	Fill &setLocked(bool locked);
	Fill &setHelperOnly(bool helperOnly);
	Fill *reference();
};
struct Circle: public Box {
	Circle(std::string_view name, float x, float y, float width, float height);
	virtual ~Circle();
	virtual void draw(Context &context, const math::Rectanglef &parentRect) override;
	Circle &setStyle(common::Ref<Style> style);
	Circle &setLocked(bool locked);
	Circle &setHelperOnly(bool helperOnly);
	Circle *reference();
	virtual bool hitTest(const math::Vector2f &point) const override;
};
struct Pie: public Box {
	Pie(std::string_view name, float x, float y, float width, float height);
	virtual ~Pie();
	virtual void draw(Context &context, const math::Rectanglef &parentRect) override;
	Pie &setStartAngle(float startAngle);
	Pie &setEndAngle(float endAngle);
	Pie &setStyle(common::Ref<Style> style);
	Pie &setLocked(bool locked);
	Pie &setHelperOnly(bool helperOnly);
	Pie *reference();
	virtual bool hitTest(const math::Vector2f &point) const override;
private:
	float m_startAngle, m_endAngle;
};
struct AspectRatio: public Box {
	AspectRatio(std::string_view name, float x, float y, float width, float height);
	virtual ~AspectRatio();
	virtual void draw(Context &context, const math::Rectanglef &parentRect) override;
	AspectRatio &setAspectRatio(float aspectRatio);
	AspectRatio &setStyle(common::Ref<Style> style);
	AspectRatio &setLocked(bool locked);
	AspectRatio &setHelperOnly(bool helperOnly);
	AspectRatio *reference();
private:
	float m_aspectRatio;
};
}
#endif /* GPICK_LAYOUT_BOX_H_ */
