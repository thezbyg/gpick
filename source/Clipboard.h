/*
 * Copyright (c) 2009-2020, Albertas Vyšniauskas
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

#ifndef GPICK_CLIPBOARD_H_
#define GPICK_CLIPBOARD_H_
#include "Converters.h"
#include "common/Ref.h"
#include <string>
#include <vector>
#include <variant>
struct ColorObject;
struct GlobalState;
struct Converter;
struct Color;
struct ColorList;
typedef struct _GtkWidget GtkWidget;
namespace clipboard {
void set(const std::string &value);
using ConverterSelection = std::variant<const char *, Converter *, Converters::Type>;
void set(const ColorObject *colorObject, GlobalState &gs, ConverterSelection converterSelection);
void set(const ColorObject &colorObject, GlobalState &gs, ConverterSelection converterSelection);
void set(const std::vector<ColorObject> &colorObjects, GlobalState &gs, ConverterSelection converterSelection);
void set(const Color &color, GlobalState &gs, ConverterSelection converterSelection);
void set(GtkWidget *paletteWidget, GlobalState &gs, ConverterSelection converterSelection);
bool colorObjectAvailable();
[[nodiscard]] common::Ref<ColorObject> getFirst(GlobalState &gs);
[[nodiscard]] common::Ref<ColorList> getColors(GlobalState &gs);
}
#endif /* GPICK_CLIPBOARD_H_ */
