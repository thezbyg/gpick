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

#include "Xml.h"
#include "Map.h"
#include "Variable.h"
#include "Types.h"
#include "common/Scoped.h"
#include <expat.h>
#include <sstream>
using namespace std::string_literals;
namespace dynv {
namespace xml {
using ValueType = types::ValueType;
bool writeStart(std::ostream &stream, const std::string &name) {
	stream << "<" << name << ">";
	return stream.good();
}
bool writeStart(std::ostream &stream, const std::string &name, const std::string &type) {
	stream << "<" << name << " type=\"" << type << "\">";
	return stream.good();
}
bool writeEnd(std::ostream &stream, const std::string &name) {
	stream << "</" << name << ">";
	return stream.good();
}
bool writeListStart(std::ostream &stream, const std::string &name, const std::string &type) {
	stream << "<" << name << " type=\"" << type << "\" list=\"true\">";
	return stream.good();
}
struct SerializeVisitor {
	SerializeVisitor(std::ostream &stream, const std::string &name):
		stream(stream),
		name(name) {
	}
	template<typename T>
	bool operator()(const T &value) const {
		if (!writeStart(stream, name, dynv::types::typeHandler<T>().name))
			return false;
		if (!types::xml::write(stream, value))
			return false;
		if (!writeEnd(stream, name))
			return false;
		return true;
	}
	bool operator()(const common::Ref<Map> &value) const {
		if (!writeStart(stream, name, dynv::types::typeHandler<common::Ref<Map>>().name))
			return false;
		if (!types::xml::write(stream, value))
			return false;
		if (!writeEnd(stream, name))
			return false;
		return true;
	}
	template<typename T>
	bool operator()(const std::vector<T> &values) const {
		using namespace std::string_literals;
		if (!writeListStart(stream, name, dynv::types::typeHandler<T>().name))
			return false;
		for (const auto &i: values) {
			if (!writeStart(stream, "li"s))
				return false;
			if (!types::xml::write(stream, i))
				return false;
			if (!writeEnd(stream, "li"s))
				return false;
		}
		return writeEnd(stream, name);
	}
	std::ostream &stream;
	const std::string &name;
};
static bool isTrue(const char *value) {
	using namespace std::string_literals;
	if (value == nullptr)
		return false;
	return value == "true"s;
}
enum class EntityType {
	root,
	map,
	list,
	listItem,
	value,
	unknown,
};
struct Entity {
	Entity(Entity &entity) = delete;
	Entity(Entity &&entity):
		m_system(entity.m_system),
		m_entityType(entity.m_entityType),
		m_valueType(entity.m_valueType),
		m_value(std::move(entity.m_value)) {
	}
	Entity(Map &map, EntityType entityType, ValueType valueType):
		m_system(map),
		m_entityType(entityType),
		m_valueType(valueType) {
	}
	Entity(Map &map, EntityType entityType, ValueType valueType, std::unique_ptr<Variable> &&value):
		m_system(map),
		m_entityType(entityType),
		m_valueType(valueType),
		m_value(std::move(value)) {
	}
	void write(const XML_Char *data, int length) {
		m_data.write(reinterpret_cast<const char *>(data), length);
	}
	std::string data() {
		return m_data.str();
	}
	bool isList() const {
		return m_entityType == EntityType::list;
	}
	bool ignoreData() const {
		return m_entityType != EntityType::listItem && m_entityType != EntityType::value;
	}
	bool isIgnored() const {
		return m_entityType == EntityType::unknown;
	}
	bool hasValue() const {
		return static_cast<bool>(m_value);
	}
	std::unique_ptr<Variable> &value() {
		return m_value;
	}
	Map &map() {
		return m_system;
	}
	ValueType valueType() const {
		return m_valueType;
	}
	EntityType entityType() const {
		return m_entityType;
	}
private:
	Map &m_system;
	std::stringstream m_data;
	EntityType m_entityType;
	ValueType m_valueType;
	std::unique_ptr<Variable> m_value;
};
struct Context {
	Context(Map &map):
		m_rootFound(false),
		m_errors(0) {
		push(map, EntityType::root);
	}
	~Context() {
		m_entities.clear();
	}
	Entity &entity() {
		return m_entities.back();
	}
	Entity &parentEntity() {
		return m_entities.at(m_entities.size() - 2);
	}
	bool rootFound() const {
		return m_rootFound;
	}
	void setRootFound() {
		m_rootFound = true;
	}
	void push(Map &map, EntityType entityType, ValueType valueType = ValueType::unknown) {
		m_entities.emplace_back(map, entityType, valueType);
	}
	void push(Map &map, EntityType entityType, ValueType valueType, std::unique_ptr<Variable> &&value) {
		m_entities.emplace_back(map, entityType, valueType, std::move(value));
	}
	void pop() {
		m_entities.pop_back();
	}
	void error() {
		m_errors++;
	}
	operator bool() const {
		return m_errors == 0;
	}
private:
	bool m_rootFound;
	std::vector<Entity> m_entities;
	uint32_t m_errors;
};
static const char *getAttribute(const XML_Char **attributes, const std::string &name) {
	for (auto i = attributes; *i; i += 2) {
		if (*i == name)
			return i[1];
	}
	return nullptr;
}
static void onCharacterData(Context *context, const XML_Char *data, int length) {
	auto &entity = context->entity();
	if (entity.ignoreData() || entity.isIgnored())
		return;
	entity.write(data, length);
}
struct IsVector {
	IsVector(Entity &entity):
		entity(entity) {
	}
	template<typename T>
	bool operator()(const T &) const {
		return false;
	}
	template<typename T>
	bool operator()(const std::vector<T> &) const {
		return true;
	}
	Entity &entity;
};
static void onStartElement(Context *context, const XML_Char *name, const XML_Char **attributes) {
	using namespace std::string_literals;
	if (!context->rootFound()) {
		if (name == "root"s)
			context->setRootFound();
		return;
	}
	auto &entity = context->entity();
	if (entity.isIgnored()) {
		context->push(entity.map(), EntityType::unknown);
		return;
	}
	if (entity.isList()) {
		if (!(name && name == "li"s)) {
			context->push(entity.map(), EntityType::unknown);
			return;
		}
		if (!std::visit(IsVector(entity), entity.value()->data())) {
			context->error();
			context->push(entity.map(), EntityType::unknown);
			return;
		}
		if (entity.valueType() == ValueType::map) {
			auto map = common::Ref<Map>(new Map());
			auto &data = entity.value()->data();
			context->push(*map, EntityType::listItem);
			std::get<std::vector<common::Ref<Map>>>(data).push_back(std::move(map));
		} else {
			context->push(entity.map(), EntityType::listItem);
		}
		return;
	}
	auto type = types::stringToType(getAttribute(attributes, "type"s));
	auto isList = isTrue(getAttribute(attributes, "list"s));
	switch (type) {
	case ValueType::basicBool:
	case ValueType::basicFloat:
	case ValueType::basicInt32:
	case ValueType::color:
	case ValueType::string:
		if (isList) {
			std::unique_ptr<Variable> variable;
			switch (type) {
			case ValueType::basicBool:
				variable = std::make_unique<Variable>(name, std::vector<bool>());
				break;
			case ValueType::basicFloat:
				variable = std::make_unique<Variable>(name, std::vector<float>());
				break;
			case ValueType::basicInt32:
				variable = std::make_unique<Variable>(name, std::vector<int32_t>());
				break;
			case ValueType::color:
				variable = std::make_unique<Variable>(name, std::vector<Color>());
				break;
			case ValueType::string:
				variable = std::make_unique<Variable>(name, std::vector<std::string>());
				break;
			case ValueType::map:
			case ValueType::unknown:
				break;
			}
			context->push(entity.map(), EntityType::list, type, std::move(variable));
		} else {
			context->push(entity.map(), EntityType::value, type);
		}
		break;
	case ValueType::map:
		if (isList) {
			auto systems = std::vector<common::Ref<Map>>();
			auto variable = std::make_unique<Variable>(name, systems);
			context->push(entity.map(), EntityType::list, type, std::move(variable));
		} else {
			auto map = common::Ref<Map>(new Map());
			entity.map().set(name, map);
			context->push(*map, EntityType::map, type);
		}
		break;
	case ValueType::unknown:
		context->push(entity.map(), EntityType::unknown);
		break;
	}
}
static void onEndElement(Context *context, const XML_Char *name) {
	if (!context->rootFound())
		return;
	auto &entity = context->entity();
	if (entity.isIgnored()) {
		context->pop();
		return;
	}
	if (entity.isList()) {
		entity.map().set(std::move(entity.value()));
		context->pop();
		return;
	}
	if (entity.entityType() == EntityType::listItem) {
		auto &listEntity = context->parentEntity();
		auto &data = listEntity.value()->data();
		switch (listEntity.valueType()) {
		case ValueType::string:
			std::get<std::vector<std::string>>(data).push_back(entity.data());
			break;
		case ValueType::basicBool:
			std::get<std::vector<bool>>(data).push_back(entity.data() == "true");
			break;
		case ValueType::basicInt32:
			std::get<std::vector<int32_t>>(data).push_back(std::stoi(entity.data()));
			break;
		case ValueType::basicFloat:
			std::get<std::vector<float>>(data).push_back(std::stof(entity.data()));
			break;
		case ValueType::color: {
			std::stringstream in(entity.data());
			Color color;
			in >> color[0] >> color[1] >> color[2] >> color[3];
			std::get<std::vector<Color>>(data).push_back(color);
		} break;
		case ValueType::map:
			break;
		case ValueType::unknown:
			break;
		}
		context->pop();
		return;
	}
	if (entity.entityType() == EntityType::value) {
		auto &map = entity.map();
		switch (entity.valueType()) {
		case ValueType::string:
			map.set(name, entity.data());
			break;
		case ValueType::basicBool:
			map.set(name, entity.data() == "true");
			break;
		case ValueType::basicInt32:
			map.set(name, std::stoi(entity.data()));
			break;
		case ValueType::basicFloat:
			map.set(name, std::stof(entity.data()));
			break;
		case ValueType::color: {
			std::stringstream in(entity.data());
			Color color;
			in >> color[0] >> color[1] >> color[2] >> color[3];
			map.set(name, color);
		} break;
		case ValueType::map:
			break;
		case ValueType::unknown:
			break;
		}
		context->pop();
		return;
	}
	if (entity.entityType() == EntityType::map) {
		context->pop();
		return;
	}
}
bool serialize(std::ostream &stream, const Map &map, bool addRootElement) {
	if (addRootElement) {
		stream << "<?xml version=\"1.0\" encoding=\"UTF-8\"?><root>";
		if (!stream.good())
			return false;
	}
	auto visitor = [&stream](const Variable &value) -> bool {
		if (!std::visit(SerializeVisitor(stream, value.name()), value.data()))
			return false;
		return true;
	};
	if (!map.visit(visitor))
		return false;
	if (addRootElement) {
		stream << "</root>";
		if (!stream.good())
			return false;
	}
	return true;
}
bool deserialize(std::istream &stream, Map &map) {
	auto parser = XML_ParserCreate("UTF-8");
	common::Scoped freeParser(XML_ParserFree, parser);
	XML_SetElementHandler(parser, reinterpret_cast<XML_StartElementHandler>(onStartElement), reinterpret_cast<XML_EndElementHandler>(onEndElement));
	XML_SetCharacterDataHandler(parser, reinterpret_cast<XML_CharacterDataHandler>(onCharacterData));
	xml::Context context(map);
	XML_SetUserData(parser, &context);
	for (;;) {
		auto buffer = XML_GetBuffer(parser, 4096);
		stream.read(reinterpret_cast<char *>(buffer), 4096);
		size_t length = stream.gcount();
		if (!XML_ParseBuffer(parser, length, length == 0)) {
			std::cerr << "XML parse error\n";
			return false;
		}
		if (length == 0) break;
	}
	return context;
}
}
}
