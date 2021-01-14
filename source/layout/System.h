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

#ifndef GPICK_LAYOUT_SYSTEM_H_
#define GPICK_LAYOUT_SYSTEM_H_
#include "Box.h"
#include "Style.h"
#include "Context.h"
#include "ReferenceCounter.h"
#include "math/Rectangle.h"
#include "math/Vector.h"
#include <gtk/gtk.h>
#ifndef _MSC_VER
#include <stdbool.h>
#endif
#include <stdint.h>
#include <vector>
#include <string>
namespace layout {
struct System: public ReferenceCounter {
	System();
	virtual ~System();
	void draw(Context *context, const math::Rectangle<float> &parentRect);
	Box *getBoxAt(const math::Vector2f &point);
	Box *getNamedBox(const char *name);
	Box *getNamedBox(const std::string &name);
	void addStyle(Style *style);
	void setBox(Box *box);
	const std::vector<Style *> &styles() const;
	Box *box();
private:
	std::vector<Style *> m_styles;
	Box *m_box;
};
}
#endif /* GPICK_LAYOUT_SYSTEM_H_ */
