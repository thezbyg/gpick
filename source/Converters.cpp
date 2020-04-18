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

#include "Converters.h"
#include "Converter.h"
#include "ColorObject.h"
#include <map>
#include <set>
using namespace std;
Converters::Converters()
{
}
Converters::~Converters()
{
	for (auto converter: m_all_converters){
		delete converter;
	}
}
void Converters::add(Converter *converter)
{
	m_all_converters.push_back(converter);
	if (converter->copy() && converter->hasSerialize())
		m_copy_converters.push_back(converter);
	if (converter->paste() && converter->hasDeserialize())
		m_paste_converters.push_back(converter);
	m_converters[converter->name()] = converter;
}
void Converters::rebuildCopyPasteArrays()
{
	m_copy_converters.clear();
	m_paste_converters.clear();
	for (auto converter: m_all_converters){
		if (converter->copy() && converter->hasSerialize())
			m_copy_converters.push_back(converter);
		if (converter->paste() && converter->hasDeserialize())
			m_paste_converters.push_back(converter);
	}
}
const std::vector<Converter*> &Converters::all() const
{
	return m_all_converters;
}
const std::vector<Converter*> &Converters::allCopy() const
{
	return m_copy_converters;
}
bool Converters::hasCopy() const
{
	return m_copy_converters.size() != 0;
}
const std::vector<Converter*> &Converters::allPaste() const
{
	return m_paste_converters;
}
Converter *Converters::byName(const char *name) const
{
	auto i = m_converters.find(name);
	if (i != m_converters.end()){
		return i->second;
	}
	return nullptr;
}
Converter *Converters::display() const
{
	return m_display_converter;
}
Converter *Converters::colorList() const
{
	return m_color_list_converter;
}
Converter *Converters::forType(Type type) const {
	switch (type) {
	case Type::display:
		return m_display_converter;
	case Type::colorList:
		return m_color_list_converter;
	case Type::copy:
		return m_copy_converters.size() != 0 ? m_copy_converters.front() : nullptr;
	}
	return nullptr;
}
void Converters::display(const char *name)
{
	m_display_converter = byName(name);
}
void Converters::colorList(const char *name)
{
	m_color_list_converter = byName(name);
}
void Converters::display(Converter *converter)
{
	m_display_converter = converter;
}
void Converters::colorList(Converter *converter)
{
	m_color_list_converter = converter;
}
Converter *Converters::firstCopy() const
{
	if (m_copy_converters.size() == 0) return nullptr;
	return m_copy_converters.front();
}
Converter *Converters::firstCopyOrAny() const
{
	if (m_copy_converters.size() == 0){
		if (m_all_converters.size() == 0){
			return nullptr;
		}
		return m_all_converters.front();
	}
	return m_copy_converters.front();
}
Converter *Converters::byNameOrFirstCopy(const char *name) const
{
	if (name){
		Converter *result = byName(name);
		if (result) return result;
	}
	if (m_copy_converters.size() == 0) return nullptr;
	return m_copy_converters.front();
}
std::string Converters::serialize(ColorObject *color_object, Type type)
{
	Converter *converter;
	switch (type){
		case Type::colorList:
			converter = colorList();
			break;
		case Type::display:
			converter = display();
			break;
		default:
			converter = nullptr;
	}
	if (converter){
		return converter->serialize(color_object);
	}
	converter = firstCopyOrAny();
	if (converter){
		return converter->serialize(color_object);
	}
	return "";
}
std::string Converters::serialize(const Color &color, Type type)
{
	ColorObject color_object("", color);
	return serialize(&color_object, type);
}
bool Converters::deserialize(const char *value, ColorObject **output_color_object)
{
	ColorObject color_object;
	multimap<float, ColorObject*, greater<float>> results;
	if (m_display_converter){
		Converter *converter = m_display_converter;
		if (converter->hasDeserialize()){
			float quality;
			if (converter->deserialize(value, &color_object, quality)){
				if (quality > 0){
					results.insert(make_pair(quality, color_object.copy()));
				}
			}
		}
	}
	for (auto &converter: m_paste_converters){
		if (!converter->hasDeserialize())
			continue;
		float quality;
		if (converter->deserialize(value, &color_object, quality)){
			if (quality > 0){
				results.insert(make_pair(quality, color_object.copy()));
			}
		}
	}
	bool first = true;
	for (auto result: results){
		if (first){
			first = false;
			*output_color_object = result.second;
		}else{
			result.second->release();
		}
	}
	if (first){
		return false;
	}else{
		return true;
	}
}
void Converters::reorder(const char **names, size_t count)
{
	set<Converter*> used;
	vector<Converter*> converters;
	for (size_t i = 0; i < count; i++){
		auto converter = byName(names[i]);
		if (converter){
			used.insert(converter);
			converters.push_back(converter);
		}
	}
	for (auto converter: m_all_converters){
		if (used.count(converter) == 0){
			converters.push_back(converter);
		}
	}
	m_all_converters.clear();
	m_all_converters = converters;
}
