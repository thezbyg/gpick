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

#include "Binary.h"
#include "Map.h"
#include "Types.h"
#include "Variable.h"
namespace dynv {
namespace binary {
using ValueType = types::ValueType;
struct CountVisitor {
	CountVisitor(const std::unordered_map<types::ValueType, uint8_t> &typeMap):
		typeMap(typeMap) {
	}
	template<typename T>
	int operator()(const T &value) const {
		using namespace types::binary;
		auto i = typeMap.find(dynv::types::typeHandler<T>().type);
		return i != typeMap.end() ? 1 : 0;
	}
	int operator()(const Ref &value) const {
		return 0;
	}
	template<typename T>
	int operator()(const std::vector<T> &values) const {
		return 0;
	}
	const std::unordered_map<types::ValueType, uint8_t> &typeMap;
};
struct SerializeVisitor {
	SerializeVisitor(std::ostream &stream, const std::string &name, const std::unordered_map<types::ValueType, uint8_t> &typeMap):
		stream(stream),
		name(name),
		typeMap(typeMap) {
	}
	template<typename T>
	bool operator()(const T &value) const {
		using namespace types::binary;
		auto i = typeMap.find(dynv::types::typeHandler<T>().type);
		if (i == typeMap.end())
			return true;
		if (!write(stream, i->second))
			return false;
		if (!write(stream, name))
			return false;
		if (!write(stream, value))
			return false;
		return true;
	}
	bool operator()(const Ref &value) const {
		return true; // skip unsupported value type for binary serialization
	}
	template<typename T>
	bool operator()(const std::vector<T> &values) const {
		return true; // skip unsupported value type for binary serialization
	}
	std::ostream &stream;
	const std::string &name;
	const std::unordered_map<types::ValueType, uint8_t> &typeMap;
};
bool serialize(std::ostream &stream, const Map &map, const std::unordered_map<types::ValueType, uint8_t> &typeMap) {
	using namespace types::binary;
	uint32_t count = 0;
	auto countVisitor = [&typeMap, &count](const Variable &value) -> bool {
		count += std::visit(CountVisitor(typeMap), value.data());
		return true;
	};
	if (!map.visit(countVisitor))
		return false;
	if (!write(stream, count))
		return false;
	auto visitor = [&stream, &typeMap](const Variable &value) -> bool {
		if (!std::visit(SerializeVisitor(stream, value.name(), typeMap), value.data()))
			return false;
		return true;
	};
	if (!map.visit(visitor))
		return false;
	return true;
}
bool deserialize(std::istream &stream, Map &map, const std::unordered_map<uint8_t, ValueType> &typeMap) {
	using namespace types::binary;
	uint32_t count = read<uint32_t>(stream);
	if (!stream.good())
		return false;
	for (uint32_t i = 0; i < count; i++) {
		uint8_t handlerId = read<uint8_t>(stream);
		if (!stream.good())
			return false;
		std::string name = read<std::string>(stream);
		if (!stream.good())
			return false;
		auto type = typeMap.find(handlerId);
		if (type == typeMap.end()) {
			auto skip = read<uint32_t>(stream);
			if (!stream.good())
				return false;
			stream.seekg(skip, std::ios::cur);
			continue;
		}
		switch (type->second) {
		case ValueType::basicBool: {
			auto value = read<uint8_t>(stream);
			if (!stream.good())
				return false;
			map.set<bool>(name, value);
		} break;
		case ValueType::basicFloat: {
			auto value = read<float>(stream);
			if (!stream.good())
				return false;
			map.set(name, value);
		} break;
		case ValueType::basicInt32: {
			auto value = read<int32_t>(stream);
			if (!stream.good())
				return false;
			map.set(name, value);
		} break;
		case ValueType::string: {
			auto value = read<std::string>(stream);
			if (!stream.good())
				return false;
			map.set(name, value);
		} break;
		case ValueType::color: {
			auto value = read<Color>(stream);
			if (!stream.good())
				return false;
			map.set(name, value);
		} break;
		case ValueType::map:
		case ValueType::unknown:
			return false;
		}
	}
	return true;
}
}
}
