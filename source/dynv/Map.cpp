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

#include "Map.h"
#include "Variable.h"
#include "Xml.h"
#include "Binary.h"
#include "Types.h"
#include <vector>
#include <queue>
#include <iostream>
#include <type_traits>
namespace dynv {
template<typename T, typename std::enable_if_t<!std::is_reference<T>::value, int> = 0>
auto get(const Map &map, const std::string &name, T defaultValue) {
	bool valid;
	std::string fieldName;
	auto &values = map.valuesForPath(name, valid, fieldName);
	if (!valid)
		return defaultValue;
	auto i = values.find(fieldName);
	if (i == values.end())
		return defaultValue;
	auto &data = (*i)->data();
	if (data.type() != typeid(T))
		return defaultValue;
	return boost::get<T>(data);
}
template<typename T, typename std::enable_if_t<std::is_reference<T>::value, int> = 0>
auto get(const Map &map, const std::string &name, const T &defaultValue) {
	bool valid;
	std::string fieldName;
	auto &values = map.valuesForPath(name, valid, fieldName);
	if (!valid)
		return defaultValue;
	auto i = values.find(fieldName);
	if (i == values.end())
		return defaultValue;
	auto &data = (*i)->data();
	if (data.type() != typeid(T))
		return defaultValue;
	return boost::get<T>(data);
}
template<typename T>
auto get(const Map &map, const std::string &name) {
	bool valid;
	std::string fieldName;
	auto &values = map.valuesForPath(name, valid, fieldName);
	if (!valid)
		return T();
	auto i = values.find(fieldName);
	if (i == values.end())
		return T();
	auto &data = (*i)->data();
	if (data.type() != typeid(T))
		return T();
	return boost::get<T>(data);
}
template<typename T>
auto getVector(const Map &map, const std::string &name) {
	bool valid;
	std::string fieldName;
	auto &values = map.valuesForPath(name, valid, fieldName);
	if (!valid)
		return std::vector<T>();
	auto i = values.find(fieldName);
	if (i == values.end())
		return std::vector<T>();
	auto &data = (*i)->data();
	if (data.type() != typeid(std::vector<T>)) {
		if (data.type() != typeid(T)) // try to fallback to non-vector type
			return std::vector<T>();
		return std::vector<T> { boost::get<T>(data) };
	}
	return boost::get<std::vector<T>>(data);
}
bool Map::getBool(const std::string &name, bool defaultValue) const {
	return get(*this, name, defaultValue);
}
float Map::getFloat(const std::string &name, float defaultValue) const {
	return get(*this, name, defaultValue);
}
int32_t Map::getInt32(const std::string &name, int32_t defaultValue) const {
	return get(*this, name, defaultValue);
}
Color Map::getColor(const std::string &name, Color defaultValue) const {
	return get(*this, name, defaultValue);
}
std::string Map::getString(const std::string &name, const std::string &defaultValue) const {
	return get(*this, name, defaultValue);
}
std::vector<bool> Map::getBools(const std::string &name) const {
	return getVector<bool>(*this, name);
}
std::vector<float> Map::getFloats(const std::string &name) const {
	return getVector<float>(*this, name);
}
std::vector<int32_t> Map::getInt32s(const std::string &name) const {
	return getVector<int32_t>(*this, name);
}
std::vector<Color> Map::getColors(const std::string &name) const {
	return getVector<Color>(*this, name);
}
std::vector<std::string> Map::getStrings(const std::string &name) const {
	return getVector<std::string>(*this, name);
}
Ref Map::getMap(const std::string &name) {
	return get<Ref>(*this, name);
}
Ref Map::getOrCreateMap(const std::string &name) {
	bool valid;
	std::string fieldName;
	auto &values = valuesForPath(name, valid, fieldName, true);
	if (!valid)
		return Ref();
	auto i = values.find(fieldName);
	if (i == values.end()) {
		Ref result;
		values.emplace(new Variable(fieldName, (result = create())));
		return result;
	}
	auto &data = (*i)->data();
	if (data.type() != typeid(Ref)) {
		Ref result;
		(*i)->assign((result = create()));
		return result;
	}
	return boost::get<Ref &>(data);
}
const Ref Map::getMap(const std::string &name) const {
	return get<Ref>(*this, name);
}
std::vector<Ref> Map::getMaps(const std::string &name) {
	bool valid;
	std::string fieldName;
	auto &values = valuesForPath(name, valid, fieldName, true);
	if (!valid)
		return std::vector<Ref>();
	auto i = values.find(fieldName);
	if (i == values.end())
		return std::vector<Ref>();
	auto &data = (*i)->data();
	if (data.type() != typeid(std::vector<Ref>)) {
		if (data.type() != typeid(Ref)) // try to fallback to non-vector type
			return std::vector<Ref>();
		auto result = std::vector<Ref>();
		result.emplace_back(boost::get<Ref &>(data));
		return result;
	}
	return boost::get<std::vector<Ref>>(data);
}
std::vector<Ref> Map::getMaps(const std::string &name) const {
	bool valid;
	std::string fieldName;
	auto &values = valuesForPath(name, valid, fieldName);
	if (!valid)
		return std::vector<Ref>();
	auto i = values.find(fieldName);
	if (i == values.end())
		return std::vector<Ref>();
	auto &data = (*i)->data();
	if (data.type() != typeid(std::vector<Ref>)) {
		if (data.type() != typeid(Ref)) // try to fallback to non-vector type
			return std::vector<Ref>();
		auto result = std::vector<Ref>();
		result.emplace_back(boost::get<Ref &>(data));
		return result;
	}
	return boost::get<std::vector<Ref>>(data);
}
struct IsMap: public boost::static_visitor<bool> {
	template<typename T>
	bool operator()(const T &) const {
		return false;
	}
	bool operator()(const Ref &value) const {
		return true;
	}
};
const Map::Set &Map::valuesForPath(const std::string &path, bool &valid, std::string &name) const {
	size_t position = path.find('.');
	if (position == std::string::npos) {
		name = path;
		valid = true;
		return m_values;
	}
	size_t from = 0;
	auto pathPart = path.substr(from, position);
	Ref next;
	auto i = m_values.find(pathPart);
	if (i == m_values.end()) {
		valid = false;
		return m_values;
	} else {
		if (!boost::apply_visitor(IsMap(), (*i)->data())) {
			valid = false;
			return m_values;
		}
		next = boost::get<Ref &>((*i)->data());
		if (!next) {
			valid = false;
			return m_values;
		}
	}
	for (;;) {
		from = position + 1;
		position = path.find('.', from);
		if (position == std::string::npos) {
			name = path.substr(from);
			valid = true;
			return next->m_values;
		}
		pathPart = path.substr(from, position - from);
		i = next->m_values.find(pathPart);
		if (i == next->m_values.end()) {
			valid = false;
			return m_values;
		} else {
			if (!boost::apply_visitor(IsMap(), (*i)->data())) {
				valid = false;
				return m_values;
			}
			next = boost::get<Ref &>((*i)->data());
			if (!next) {
				valid = false;
				return m_values;
			}
		}
	}
}
Map::Set &Map::valuesForPath(const std::string &path, bool &valid, std::string &name, bool createMissing) {
	size_t position = path.find('.');
	if (position == std::string::npos) {
		name = path;
		valid = true;
		return m_values;
	}
	size_t from = 0;
	auto pathPart = path.substr(from, position);
	Ref next;
	auto i = m_values.find(pathPart);
	if (i == m_values.end()) {
		if (!createMissing) {
			valid = false;
			return m_values;
		}
		m_values.emplace(new Variable(pathPart, (next = create())));
	} else {
		if (!boost::apply_visitor(IsMap(), (*i)->data())) {
			valid = false;
			return m_values;
		}
		next = boost::get<Ref &>((*i)->data());
		if (!next) {
			if (!createMissing) {
				valid = false;
				return m_values;
			}
			m_values.emplace(new Variable(pathPart, (next = create())));
		}
	}
	for (;;) {
		from = position + 1;
		position = path.find('.', from);
		if (position == std::string::npos) {
			name = path.substr(from);
			valid = true;
			return next->m_values;
		}
		pathPart = path.substr(from, position - from);
		i = next->m_values.find(pathPart);
		if (i == next->m_values.end()) {
			if (!createMissing) {
				valid = false;
				return m_values;
			}
			next->m_values.emplace(new Variable(pathPart, (next = create())));
		} else {
			if (!boost::apply_visitor(IsMap(), (*i)->data())) {
				valid = false;
				return m_values;
			}
			next = boost::get<Ref &>((*i)->data());
			if (!next) {
				if (!createMissing) {
					valid = false;
					return m_values;
				}
				next->m_values.emplace(new Variable(pathPart, (next = create())));
			}
		}
	}
}
template<typename T>
Map &setByPath(Map &map, const std::string &name, T value) {
	bool valid;
	std::string fieldName;
	auto &values = map.valuesForPath(name, valid, fieldName, true);
	if (valid) {
		auto i = values.find(fieldName);
		if (i == values.end())
			values.emplace(new Variable(fieldName, value));
		else {
			(*i)->assign(value);
		}
	}
	return map;
}
template<typename T>
Map &setByPath(Map &map, const std::string &name, common::Span<T> value) {
	bool valid;
	std::string fieldName;
	auto &values = map.valuesForPath(name, valid, fieldName, true);
	if (valid) {
		auto i = values.find(fieldName);
		if (i == values.end())
			values.emplace(new Variable(fieldName, std::vector<T>(value.begin(), value.end())));
		else {
			(*i)->assign(std::vector<T>(value.begin(), value.end()));
		}
	}
	return map;
}
Map &Map::set(const std::string &name, bool value) {
	return setByPath(*this, name, value);
}
Map &Map::set(const std::string &name, float value) {
	return setByPath(*this, name, value);
}
Map &Map::set(const std::string &name, int32_t value) {
	return setByPath(*this, name, value);
}
Map &Map::set(const std::string &name, const Color &value) {
	return setByPath(*this, name, value);
}
Map &Map::set(const std::string &name, const std::string &value) {
	return setByPath(*this, name, value);
}
Map &Map::set(const std::string &name, const char *value) {
	return setByPath(*this, name, value);
}
Map &Map::set(const std::string &name, Ref value) {
	return setByPath(*this, name, value);
}
Map &Map::set(const std::string &name, const std::vector<bool> &values) {
	return setByPath(*this, name, values);
}
Map &Map::set(const std::string &name, const std::vector<float> &values) {
	return setByPath(*this, name, values);
}
Map &Map::set(const std::string &name, const std::vector<int32_t> &values) {
	return setByPath(*this, name, values);
}
Map &Map::set(const std::string &name, const std::vector<Color> &values) {
	return setByPath(*this, name, values);
}
Map &Map::set(const std::string &name, const std::vector<std::string> &values) {
	return setByPath(*this, name, values);
}
Map &Map::set(const std::string &name, const std::vector<const char *> &values) {
	return setByPath(*this, name, values);
}
Map &Map::set(const std::string &name, const std::vector<Ref> &values) {
	return setByPath(*this, name, values);
}
Map &Map::set(const std::string &name, const common::Span<bool> values) {
	return setByPath(*this, name, values);
}
Map &Map::set(const std::string &name, const common::Span<float> values) {
	return setByPath(*this, name, values);
}
Map &Map::set(const std::string &name, const common::Span<int32_t> values) {
	return setByPath(*this, name, values);
}
Map &Map::set(const std::string &name, const common::Span<Color> values) {
	return setByPath(*this, name, values);
}
Map &Map::set(const std::string &name, const common::Span<std::string> values) {
	return setByPath(*this, name, values);
}
Map &Map::set(const std::string &name, const common::Span<const char *> values) {
	return setByPath(*this, name, values);
}
Map &Map::set(const std::string &name, const common::Span<Ref> values) {
	return setByPath(*this, name, values);
}
Map &Map::set(std::unique_ptr<Variable> &&value) {
	if (!value)
		return *this;
	auto i = m_values.find(value->name());
	if (i == m_values.end())
		m_values.emplace(std::move(value));
	else
		(*i)->data() = std::move(value->data());
	return *this;
}
bool Map::remove(const std::string &name) {
	bool valid;
	std::string fieldName;
	auto &values = valuesForPath(name, valid, fieldName, false);
	if (!valid)
		return false;
	auto i = values.find(fieldName);
	if (i == values.end())
		return false;
	values.erase(i);
	return true;
}
bool Map::removeAll() {
	bool result = !m_values.empty();
	m_values.clear();
	return result;
}
size_t Map::size() const {
	return m_values.size();
}
bool Map::contains(const std::string &name) const {
	bool valid;
	std::string fieldName;
	auto &values = valuesForPath(name, valid, fieldName);
	if (!valid)
		return false;
	return values.find(fieldName) != values.end();
}
struct TypeNameVisitor: public boost::static_visitor<std::string> {
	template<typename T>
	std::string operator()(const T &) const {
		return dynv::types::typeHandler<T>().name;
	}
	template<typename T>
	std::string operator()(const std::vector<T> &) const {
		return dynv::types::typeHandler<T>().name;
	}
};
std::string Map::type(const std::string &name) const {
	auto i = m_values.find(name);
	if (i == m_values.end())
		return "";
	return boost::apply_visitor(TypeNameVisitor(), (*i)->data());
}
bool Map::serialize(std::ostream &stream, const std::unordered_map<types::ValueType, uint8_t> &typeMap) const {
	return binary::serialize(stream, *this, typeMap);
}
bool Map::deserialize(std::istream &stream, const std::unordered_map<uint8_t, types::ValueType> &typeMap) {
	return binary::deserialize(stream, *this, typeMap);
}
bool Map::serializeXml(std::ostream &stream) const {
	return xml::serialize(stream, *this);
}
bool Map::deserializeXml(std::istream &stream) {
	removeAll();
	return xml::deserialize(stream, *this);
}
bool Map::visit(std::function<bool(const Variable &value)> visitor, bool recursive) const {
	if (!recursive) {
		for (const auto &value: m_values)
			if (!visitor(*value))
				return false;
		return true;
	}
	std::queue<const Map *> systems;
	for (const auto &value: m_values) {
		if (!visitor(*value))
			return false;
		if (boost::apply_visitor(IsMap(), value->data()))
			systems.push(&*boost::get<const Ref &>(value->data()));
	}
	while (!systems.empty()) {
		auto &map = *systems.front();
		systems.pop();
		for (const auto &value: map.m_values) {
			if (!visitor(*value))
				return false;
			if (boost::apply_visitor(IsMap(), value->data()))
				systems.push(&*boost::get<const Ref &>(value->data()));
		}
	}
	return true;
}
Map::Map() {
}
Map::~Map() {
}
Ref Map::create() {
	return Ref(new Map());
}
bool Map::Compare::operator()(const std::unique_ptr<Variable> &a, const std::unique_ptr<Variable> &b) const {
	return a->name() < b->name();
}
bool Map::Compare::operator()(const std::string &name, const std::unique_ptr<Variable> &b) const {
	return name < b->name();
}
bool Map::Compare::operator()(const std::unique_ptr<Variable> &a, const std::string &name) const {
	return a->name() < name;
}
}
