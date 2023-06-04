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

#ifndef GPICK_DYNV_MAP_H_
#define GPICK_DYNV_MAP_H_
#include "MapFwd.h"
#include "Color.h"
#include "Types.h"
#include "common/Ref.h"
#include "common/Span.h"
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iosfwd>
#include <memory>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
namespace dynv {
struct Variable;
struct Map: public common::Ref<Map>::Counter {
	using Ref = common::Ref<Map>;
	struct Compare {
		using is_transparent = void;
		bool operator()(const std::unique_ptr<Variable> &a, const std::unique_ptr<Variable> &b) const;
		bool operator()(const std::string &name, const std::unique_ptr<Variable> &b) const;
		bool operator()(const std::unique_ptr<Variable> &a, const std::string &name) const;
	};
	using Set = std::set<std::unique_ptr<Variable>, Compare>;
	Map();
	virtual ~Map() override;
	bool getBool(const std::string &name, bool defaultValue = false) const;
	float getFloat(const std::string &name, float defaultValue = 0.0f) const;
	int32_t getInt32(const std::string &name, int32_t defaultValue = 0) const;
	Color getColor(const std::string &name, Color defaultValue = {}) const;
	std::string getString(const std::string &name, const std::string &defaultValue = m_defaultString) const;
	std::vector<bool> getBools(const std::string &name) const;
	std::vector<float> getFloats(const std::string &name) const;
	std::vector<int32_t> getInt32s(const std::string &name) const;
	std::vector<Color> getColors(const std::string &name) const;
	std::vector<std::string> getStrings(const std::string &name) const;
	Ref getMap(const std::string &name);
	std::vector<Ref> getMaps(const std::string &name);
	const Ref getMap(const std::string &name) const;
	std::vector<Ref> getMaps(const std::string &name) const;
	Ref getOrCreateMap(const std::string &name);
	std::vector<Ref> getOrCreateMaps(const std::string &name);
	Map &set(const std::string &name, Ref value);
	Map &set(const std::string &name, bool value);
	Map &set(const std::string &name, float value);
	Map &set(const std::string &name, int32_t value);
	Map &set(const std::string &name, const Color &value);
	Map &set(const std::string &name, const std::string &value);
	Map &set(const std::string &name, std::string_view value);
	Map &set(const std::string &name, const char *value);
	Map &set(const std::string &name, const std::vector<bool> &value);
	Map &set(const std::string &name, const std::vector<float> &value);
	Map &set(const std::string &name, const std::vector<int32_t> &value);
	Map &set(const std::string &name, const std::vector<Color> &value);
	Map &set(const std::string &name, const std::vector<std::string> &value);
	Map &set(const std::string &name, const std::vector<const char *> &value);
	Map &set(const std::string &name, const std::vector<Ref> &value);
	Map &set(const std::string &name, const common::Span<bool> value);
	Map &set(const std::string &name, const common::Span<float> value);
	Map &set(const std::string &name, const common::Span<int32_t> value);
	Map &set(const std::string &name, const common::Span<Color> value);
	Map &set(const std::string &name, const common::Span<std::string> value);
	Map &set(const std::string &name, const common::Span<const char *> value);
	Map &set(const std::string &name, const common::Span<Ref> value);
	template<typename T, typename std::enable_if_t<std::is_same<bool, T>::value || std::is_same<float, T>::value || std::is_same<int32_t, T>::value, int> = 0>
	Map &set(const std::string &name, T value) {
		return this->set(name, value);
	}
	Map &set(std::unique_ptr<Variable> &&value);
	bool remove(const std::string &name);
	bool removeAll();
	size_t size() const;
	bool contains(const std::string &name) const;
	std::string type(const std::string &name) const;
	bool serialize(std::ostream &stream, const std::unordered_map<types::ValueType, uint8_t> &typeMap) const;
	bool serializeXml(std::ostream &stream) const;
	bool deserialize(std::istream &stream, const std::unordered_map<uint8_t, types::ValueType> &typeMap);
	bool deserializeXml(std::istream &stream);
	bool visit(std::function<bool(const Variable &value)> visitor, bool recursive = false) const;
	static Ref create();
	Set &valuesForPath(const std::string &path, bool &valid, std::string &name, bool createMissing);
	const Set &valuesForPath(const std::string &path, bool &valid, std::string &name) const;
private:
	Set m_values;
	static const std::string m_defaultString;
};
}
#endif /* GPICK_DYNV_MAP_H_ */
