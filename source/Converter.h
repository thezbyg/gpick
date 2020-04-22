/*
 * Copyright (c) 2009-2017, Albertas Vyšniauskas
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

#ifndef GPICK_CONVERTER_H_
#define GPICK_CONVERTER_H_
#include <string>
#include "lua/Ref.h"
struct ColorObject;
struct Color;
struct ConverterSerializePosition
{
	ConverterSerializePosition();
	ConverterSerializePosition(size_t count);
	bool first() const;
	bool last() const;
	size_t index() const;
	size_t count() const;
	void incrementIndex();
	void first(bool value);
	void last(bool value);
	private:
	bool m_first, m_last;
	size_t m_index, m_count;
};
struct Converter
{
	Converter(const char *name, const char *label, lua::Ref &&serialize, lua::Ref &&deserialize);
	const std::string &name() const;
	const std::string &label() const;
	bool hasSerialize() const;
	bool hasDeserialize() const;
	bool copy() const;
	bool paste() const;
	void copy(bool value);
	void paste(bool value);
	std::string serialize(const ColorObject *color_object, const ConverterSerializePosition &position);
	std::string serialize(const ColorObject *color_object);
	std::string serialize(const ColorObject &colorObject, const ConverterSerializePosition &position);
	std::string serialize(const ColorObject &colorObject);
	std::string serialize(const Color &color);
	bool deserialize(const char *value, ColorObject *color_object, float &quality);
	private:
	std::string m_name;
	std::string m_label;
	lua::Ref m_serialize, m_deserialize;
	bool m_copy, m_paste;
};
#endif /* GPICK_CONVERTER_H_ */
