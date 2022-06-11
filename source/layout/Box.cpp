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

#include "Box.h"
#include "Style.h"
#include "Context.h"
#include "System.h"
#include "math/Algorithms.h"
#include <cairo/cairo.h>
#include <pango/pangocairo.h>
#include <string>
#include <typeinfo>
#include <boost/math/special_functions/round.hpp>
namespace layout {
Box::Box(std::string_view name, float x, float y, float width, float height):
	m_name(name),
	m_helperOnly(false),
	m_locked(false),
	m_rect(x, y, x + width, y + height) {
}
Box::~Box() {
}
Box &Box::setStyle(common::Ref<Style> style) {
	m_style = style;
	return *this;
}
common::Ref<Style> Box::style() {
	return m_style;
}
bool Box::locked() const {
	return m_locked;
}
Box &Box::setLocked(bool locked) {
	m_locked = locked;
	return *this;
}
bool Box::helperOnly() const {
	return m_helperOnly;
}
Box &Box::setHelperOnly(bool helperOnly) {
	m_helperOnly = helperOnly;
	return *this;
}
const math::Rectanglef &Box::rect() const {
	return m_rect;
}
void Box::draw(Context &context, const math::Rectanglef &parentRect) {
	drawChildren(context, parentRect);
}
void Box::drawChildren(Context &context, const math::Rectanglef &parentRect) {
	math::Rectanglef childRect = m_rect.impose(parentRect);
	for (auto &child: m_children) {
		if (child != context.system().selectedBox())
			child->draw(context, childRect);
	}
	for (auto &child: m_children) {
		if (child == context.system().selectedBox())
			child->draw(context, childRect);
	}
}
void Box::addChild(common::Ref<Box> box) {
	m_children.push_back(box);
}
common::Ref<Box> Box::getNamedBox(std::string_view name) {
	if (m_helperOnly)
		return common::nullRef;
	if (m_name.compare(name) == 0)
		return common::Ref<Box>(this->reference());
	common::Ref<Box> box;
	for (auto &child: m_children) {
		if ((box = child->getNamedBox(name))) {
			return box;
		}
	}
	return common::nullRef;
}
common::Ref<Box> Box::getBoxAt(const math::Vector2f &point) {
	if (!hitTest(point) || m_helperOnly)
		return common::nullRef;
	math::Vector2f transformedPoint = math::Vector2f((point.x - m_rect.getX()) / m_rect.getWidth(), (point.y - m_rect.getY()) / m_rect.getHeight());
	common::Ref<Box> box;
	for (auto &child: m_children) {
		if ((box = child->getBoxAt(transformedPoint))) {
			return box;
		}
	}
	if (typeid(*this) == typeid(Box)) //do not match Box, because it is invisible
		return common::nullRef;
	else
		return common::Ref<Box>(this->reference());
}
bool Box::hitTest(const math::Vector2f &point) const {
	return m_rect.isInside(point.x, point.y);
}
Text::Text(std::string_view name, float x, float y, float width, float height):
	Box(name, x, y, width, height) {
}
Text::~Text() {
}
Text &Text::setText(std::string_view text) {
	m_text = text;
	return *this;
}
Text &Text::setStyle(common::Ref<Style> style) {
	Box::setStyle(style);
	return *this;
}
Text &Text::setLocked(bool locked) {
	Box::setLocked(locked);
	return *this;
}
Text &Text::setHelperOnly(bool helperOnly) {
	Box::setHelperOnly(helperOnly);
	return *this;
}
Text *Text::reference() {
	return static_cast<Text *>(Counter::reference());
}
void Text::draw(Context &context, const math::Rectanglef &parentRect) {
	math::Rectanglef drawRect = rect().impose(parentRect);
	cairo_t *cr = context.getCairo();
	if (m_text != "") {
		PangoFontDescription *fontDescription = pango_font_description_new();
		pango_font_description_set_family(fontDescription, "sans-serif");
		if (helperOnly()) {
			pango_font_description_set_style(fontDescription, PANGO_STYLE_ITALIC);
		} else {
			pango_font_description_set_style(fontDescription, PANGO_STYLE_NORMAL);
		}
		math::Vector2f textOffset(0, 0);
		if (style()) {
			pango_font_description_set_absolute_size(fontDescription, style()->fontSize() * drawRect.getHeight() * PANGO_SCALE);
			Color color = style()->color();
			if (context.getTransformationChain()) {
				context.getTransformationChain()->apply(&color, &color);
			}
			cairo_set_source_rgba(cr, boost::math::round(color.rgb.red * 255.0) / 255.0, boost::math::round(color.rgb.green * 255.0) / 255.0, boost::math::round(color.rgb.blue * 255.0) / 255.0, color.alpha);
			textOffset = style()->textOffset() * math::Vector2f(drawRect.getWidth(), drawRect.getHeight());
		} else {
			pango_font_description_set_absolute_size(fontDescription, drawRect.getHeight() * PANGO_SCALE);
			cairo_set_source_rgb(cr, 0, 0, 0);
		}
		PangoLayout *layout = pango_cairo_create_layout(cr);
		pango_layout_set_font_description(layout, fontDescription);
		pango_font_description_free(fontDescription);
		pango_layout_set_wrap(layout, PANGO_WRAP_WORD_CHAR);
		pango_layout_set_alignment(layout, PANGO_ALIGN_CENTER);
		pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_END);
		pango_layout_set_width(layout, static_cast<int>(drawRect.getWidth() * PANGO_SCALE));
		pango_layout_set_height(layout, static_cast<int>(drawRect.getHeight() * PANGO_SCALE));
		pango_layout_set_text(layout, m_text.c_str(), -1);
		pango_cairo_update_layout(cr, layout);
		int width, height;
		pango_layout_get_pixel_size(layout, &width, &height);
		cairo_move_to(cr, drawRect.getX() + textOffset.x, drawRect.getY() + (drawRect.getHeight() - height) / 2 + textOffset.y);
		pango_cairo_show_layout(cr, layout);
		g_object_unref(layout);

		if (context.system().selectedBox() == this) {
			cairo_rectangle(cr, drawRect.getX() + 1, drawRect.getY() + 1, drawRect.getWidth() - 2, drawRect.getHeight() - 2);
			cairo_set_source_rgb(cr, 1, 1, 1);
			cairo_set_line_width(cr, 2);
			cairo_stroke(cr);
		}
	}
	drawChildren(context, parentRect);
}
Fill::Fill(std::string_view name, float x, float y, float width, float height):
	Box(name, x, y, width, height) {
}
Fill::~Fill() {
}
Fill &Fill::setStyle(common::Ref<Style> style) {
	Box::setStyle(style);
	return *this;
}
Fill &Fill::setLocked(bool locked) {
	Box::setLocked(locked);
	return *this;
}
Fill &Fill::setHelperOnly(bool helperOnly) {
	Box::setHelperOnly(helperOnly);
	return *this;
}
void Fill::draw(Context &context, const math::Rectanglef &parentRect) {
	math::Rectanglef drawRect = rect().impose(parentRect);
	cairo_t *cr = context.getCairo();
	Color color = style()->color();
	if (context.getTransformationChain()) {
		context.getTransformationChain()->apply(&color, &color);
	}
	cairo_set_source_rgb(cr, boost::math::round(color.rgb.red * 255.0) / 255.0, boost::math::round(color.rgb.green * 255.0) / 255.0, boost::math::round(color.rgb.blue * 255.0) / 255.0);
	cairo_rectangle(cr, drawRect.getX(), drawRect.getY(), drawRect.getWidth(), drawRect.getHeight());
	cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
	cairo_fill_preserve(cr);
	cairo_set_antialias(cr, CAIRO_ANTIALIAS_DEFAULT);
	cairo_set_line_width(cr, 1);
	cairo_stroke(cr);
	if (context.system().selectedBox() == this) {
		cairo_rectangle(cr, drawRect.getX() + 1, drawRect.getY() + 1, drawRect.getWidth() - 2, drawRect.getHeight() - 2);
		cairo_set_source_rgb(cr, 1, 1, 1);
		cairo_set_line_width(cr, 2);
		cairo_stroke(cr);
	}
	drawChildren(context, parentRect);
}
Circle::Circle(std::string_view name, float x, float y, float width, float height):
	Box(name, x, y, width, height) {
}
Circle::~Circle() {
}
Circle &Circle::setStyle(common::Ref<Style> style) {
	Box::setStyle(style);
	return *this;
}
Circle &Circle::setLocked(bool locked) {
	Box::setLocked(locked);
	return *this;
}
Circle &Circle::setHelperOnly(bool helperOnly) {
	Box::setHelperOnly(helperOnly);
	return *this;
}
void Circle::draw(Context &context, const math::Rectanglef &parentRect) {
	math::Rectanglef drawRect = rect().impose(parentRect);
	cairo_t *cr = context.getCairo();
	Color color = style()->color();
	if (context.getTransformationChain()) {
		context.getTransformationChain()->apply(&color, &color);
	}
	cairo_set_source_rgb(cr, boost::math::round(color.rgb.red * 255.0) / 255.0, boost::math::round(color.rgb.green * 255.0) / 255.0, boost::math::round(color.rgb.blue * 255.0) / 255.0);
	cairo_save(cr);
	cairo_translate(cr, drawRect.getX() + drawRect.getWidth() / 2, drawRect.getY() + drawRect.getHeight() / 2);
	cairo_scale(cr, drawRect.getWidth() / 2, drawRect.getHeight() / 2);
	cairo_arc(cr, 0, 0, 1, 0, 2 * math::PI);
	cairo_restore(cr);
	cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
	cairo_fill_preserve(cr);
	cairo_set_antialias(cr, CAIRO_ANTIALIAS_DEFAULT);
	if (context.system().selectedBox() == this) {
		cairo_set_source_rgb(cr, 1, 1, 1);
		cairo_set_line_width(cr, 2);
		cairo_stroke(cr);
	} else {
		cairo_set_line_width(cr, 1);
		cairo_stroke(cr);
	}
	drawChildren(context, parentRect);
}
bool Circle::hitTest(const math::Vector2f &point) const {
	if (!rect().isInside(point.x, point.y))
		return false;
	if (rect().getWidth() > rect().getHeight())
		return ((point - rect().center()) * 2 * math::Vector2f(rect().getHeight() / rect().getWidth(), 1.0f)).length() <= std::min(rect().getWidth(), rect().getHeight());
	else
		return ((point - rect().center()) * 2 * math::Vector2f(1.0f, rect().getWidth() / rect().getHeight())).length() <= std::min(rect().getWidth(), rect().getHeight());
}
Pie::Pie(std::string_view name, float x, float y, float width, float height):
	Box(name, x, y, width, height) {
}
Pie::~Pie() {
}
Pie &Pie::setStartAngle(float startAngle) {
	m_startAngle = startAngle;
	return *this;
}
Pie &Pie::setEndAngle(float endAngle) {
	m_endAngle = endAngle;
	return *this;
}
Pie &Pie::setStyle(common::Ref<Style> style) {
	Box::setStyle(style);
	return *this;
}
Pie &Pie::setLocked(bool locked) {
	Box::setLocked(locked);
	return *this;
}
Pie &Pie::setHelperOnly(bool helperOnly) {
	Box::setHelperOnly(helperOnly);
	return *this;
}
void Pie::draw(Context &context, const math::Rectanglef &parentRect) {
	math::Rectanglef drawRect = rect().impose(parentRect);
	cairo_t *cr = context.getCairo();
	Color color = style()->color();
	if (context.getTransformationChain()) {
		context.getTransformationChain()->apply(&color, &color);
	}
	cairo_set_source_rgb(cr, boost::math::round(color.rgb.red * 255.0) / 255.0, boost::math::round(color.rgb.green * 255.0) / 255.0, boost::math::round(color.rgb.blue * 255.0) / 255.0);
	cairo_save(cr);
	cairo_translate(cr, drawRect.getX() + drawRect.getWidth() / 2, drawRect.getY() + drawRect.getHeight() / 2);
	cairo_scale(cr, drawRect.getWidth() / 2, drawRect.getHeight() / 2);
	cairo_arc(cr, 0, 0, 1, m_startAngle * 2 * math::PI, m_endAngle * 2 * math::PI);
	cairo_line_to(cr, 0, 0);
	cairo_close_path(cr);
	cairo_restore(cr);
	cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
	cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
	cairo_fill_preserve(cr);
	cairo_set_antialias(cr, CAIRO_ANTIALIAS_DEFAULT);
	if (context.system().selectedBox() == this) {
		cairo_set_source_rgb(cr, 1, 1, 1);
		cairo_set_line_width(cr, 2);
		cairo_stroke(cr);
	} else {
		cairo_set_line_width(cr, 1);
		cairo_stroke(cr);
	}
	drawChildren(context, parentRect);
}
static bool angleIsBetween(float angle, float start, float end) {
	angle = math::wrap(angle);
	start = math::wrap(start);
	end = math::wrap(end);
	if (start < end)
		return angle >= start && angle <= end;
	else
		return angle >= start || angle <= end;
}
bool Pie::hitTest(const math::Vector2f &point) const {
	if (!rect().isInside(point.x, point.y))
		return false;
	auto pointFromCenter = point - rect().center();
	if (rect().getWidth() > rect().getHeight()) {
		if ((pointFromCenter * 2 * math::Vector2f(rect().getHeight() / rect().getWidth(), 1.0f)).length() > std::min(rect().getWidth(), rect().getHeight()))
			return false;
	} else {
		if ((pointFromCenter * 2 * math::Vector2f(1.0f, rect().getWidth() / rect().getHeight())).length() > std::min(rect().getWidth(), rect().getHeight()))
			return false;
	}
	if (pointFromCenter.length() < 0.0001f)
		return false;
	auto angle = std::atan2(pointFromCenter.y, pointFromCenter.x) / (math::PI * 2);
	if (angle < 0)
		angle = 1 + angle;
	return angleIsBetween(static_cast<float>(angle), m_startAngle, m_endAngle);
}
}
