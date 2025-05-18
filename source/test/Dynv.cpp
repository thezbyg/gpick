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

#include <boost/test/unit_test.hpp>
#include "Common.h"
#include "Color.h"
#include "dynv/Map.h"
#include <fstream>
#include <iostream>
#include <vector>
using Map = dynv::Map;
using Ref = dynv::Ref;
BOOST_AUTO_TEST_SUITE(dynv);
BOOST_AUTO_TEST_CASE(basicBool) {
	Map map;
	map.set("bool", true);
	BOOST_CHECK_EQUAL(map.getBool("bool-", false), false);
	BOOST_CHECK_EQUAL(map.getBool("bool-", true), true);
	BOOST_CHECK_EQUAL(map.getBool("bool", false), true);
	BOOST_CHECK_EQUAL(map.getBool("bool", true), true);
}
BOOST_AUTO_TEST_CASE(basicFloat) {
	Map map;
	map.set("float", 0.5f);
	BOOST_CHECK_EQUAL(map.getFloat("float-", 0.0f), 0.0f);
	BOOST_CHECK_EQUAL(map.getFloat("float-", 1.0f), 1.0f);
	BOOST_CHECK_EQUAL(map.getFloat("float", 0.0f), 0.5f);
	BOOST_CHECK_EQUAL(map.getFloat("float", 1.0f), 0.5f);
}
BOOST_AUTO_TEST_CASE(basicInt32) {
	Map map;
	map.set("int32", 5);
	BOOST_CHECK_EQUAL(map.getInt32("int32-", 0), 0);
	BOOST_CHECK_EQUAL(map.getInt32("int32-", 1), 1);
	BOOST_CHECK_EQUAL(map.getInt32("int32", 0), 5);
	BOOST_CHECK_EQUAL(map.getInt32("int32", 1), 5);
}
BOOST_AUTO_TEST_CASE(basicString) {
	Map map;
	map.set("string", "a");
	BOOST_CHECK_EQUAL(map.size(), 1u);
	BOOST_CHECK(map.contains("string"));
	BOOST_CHECK_EQUAL(map.type("string"), "string");
	BOOST_CHECK_EQUAL(map.getString("string-", "0"), "0");
	BOOST_CHECK_EQUAL(map.getString("string-", "1"), "1");
	BOOST_CHECK_EQUAL(map.getString("string", "0"), "a");
	BOOST_CHECK_EQUAL(map.getString("string", "1"), "a");
}
BOOST_AUTO_TEST_CASE(basicSystem) {
	Map map;
	Ref innerMap(new Map());
	map.set("map", innerMap);
	innerMap->set("string", "a");
	BOOST_CHECK_EQUAL(map.size(), 1u);
	BOOST_CHECK(map.contains("map"));
	auto returnedSystem = map.getMap("map-");
	BOOST_CHECK(!returnedSystem);
	returnedSystem = map.getMap("map");
	BOOST_REQUIRE(returnedSystem);
	BOOST_CHECK_EQUAL(returnedSystem->size(), 1u);
}
BOOST_AUTO_TEST_CASE(stringArray) {
	Map map;
	std::vector<std::string> data { "a", "b", "c" };
	map.set("a", data);
	auto result = map.getStrings("a");
	BOOST_REQUIRE_EQUAL(result.size(), 3u);
	for (int i = 0; i < 3; i++) {
		BOOST_CHECK_EQUAL(result[i], data[i]);
	}
}
BOOST_AUTO_TEST_CASE(constStringArray) {
	Map map;
	std::vector<const char *> data { "a", "b", "c" };
	map.set("a", data);
	auto result = map.getStrings("a");
	BOOST_REQUIRE_EQUAL(result.size(), 3u);
	for (int i = 0; i < 3; i++) {
		BOOST_CHECK_EQUAL(result[i], data[i]);
	}
}
BOOST_AUTO_TEST_CASE(stringArrayOverwrite) {
	Map map;
	std::vector<std::string> data { "a", "b", "c" };
	map.set("a", data);
	map.set("a", data);
	BOOST_CHECK_EQUAL(map.size(), 1u);
	auto result = map.getStrings("a");
	BOOST_REQUIRE_EQUAL(result.size(), 3u);
	for (int i = 0; i < 3; i++) {
		BOOST_CHECK_EQUAL(result[i], data[i]);
	}
}
BOOST_AUTO_TEST_CASE(mapArray) {
	Map map;
	std::vector<Ref> data { Ref(new Map()), Ref(new Map()), Ref(new Map()) };
	map.set("a", data);
	auto result = map.getMaps("a");
	BOOST_CHECK_EQUAL(data.size(), 3u);
	BOOST_REQUIRE_EQUAL(result.size(), 3u);
	for (int i = 0; i < 3; i++) {
		BOOST_CHECK_EQUAL(result[i], data[i]);
		BOOST_CHECK_EQUAL(result[i]->references(), 3u);
	}
}
BOOST_AUTO_TEST_CASE(noMoveOnSet) {
	Map map;
	std::vector<Ref> data { Ref(new Map()) };
	map.set("a", data);
	BOOST_CHECK_EQUAL(map.size(), 1u);
	BOOST_CHECK_EQUAL(data.size(), 1u);
}
BOOST_AUTO_TEST_CASE(xmlDeserialize) {
	Map map;
	std::ifstream file("test/config01.xml");
	BOOST_REQUIRE(file.is_open());
	BOOST_REQUIRE(map.deserializeXml(file));
	file.close();
	BOOST_CHECK_EQUAL(map.size(), 1u);
	BOOST_REQUIRE(map.contains("test"));
	auto values = map.getStrings("test");
	BOOST_REQUIRE_EQUAL(values.size(), 3u);
	const char *data[] = { "a", "b", "c" };
	for (int i = 0; i < 3; i++) {
		BOOST_CHECK_EQUAL(values[i], data[i]);
	}
}
BOOST_AUTO_TEST_CASE(xmlSerializeDeserialize) {
	Map map;
	std::vector<Ref> items;
	for (int i = 0; i < 10; i++) {
		Ref item(new Map());
		item->set("name", "a");
		item->set("color", Color(static_cast<float>(1.0f / (10 - 1) * i)));
		items.push_back(item);
	}
	map.set("items", items);
	std::stringstream output;
	map.serializeXml(output);
	auto text = output.str();
	BOOST_TEST_MESSAGE("Serialized XML: " << text);
	std::stringstream input(text);
	Map result;
	result.deserializeXml(input);
	BOOST_CHECK_EQUAL(result.size(), 1u);
	BOOST_REQUIRE(result.contains("items"));
	auto resultItems = result.getMaps("items");
	BOOST_CHECK_EQUAL(resultItems.size(), 10u);
	Color nullColor { 0 };
	for (int i = 0; i < 10; i++) {
		BOOST_CHECK_EQUAL(resultItems[i]->getString("name", ""), "a");
		BOOST_CHECK_EQUAL(resultItems[i]->getColor("color", nullColor), Color(static_cast<float>(1.0f / (10 - 1) * i)));
	}
}
BOOST_AUTO_TEST_CASE(missingPathSegmentCreationOnSet) {
	Map map;
	map.set("values.bool", true);
	BOOST_CHECK_EQUAL(map.size(), 1u);
	BOOST_CHECK(map.contains("values"));
	BOOST_CHECK(map.contains("values.bool"));
	auto values = map.getMap("values");
	BOOST_REQUIRE(values);
	BOOST_CHECK(values->contains("bool"));
	BOOST_CHECK_EQUAL(values->getBool("bool", false), true);
}
BOOST_AUTO_TEST_CASE(noMissingPathSegmentCreationOnGet) {
	Map map;
	map.getBool("values.bool", false);
	BOOST_CHECK_EQUAL(map.size(), 0u);
}
BOOST_AUTO_TEST_CASE(getOrCreateMap) {
	Map map;
	auto values = map.getOrCreateMap("values");
	BOOST_CHECK_EQUAL(map.size(), 1u);
	BOOST_CHECK(map.contains("values"));
	BOOST_REQUIRE(values);
}
BOOST_AUTO_TEST_CASE(nestedPathCreation) {
	Map map;
	map.set("a.b.c.d", "value");
	BOOST_CHECK(map.getMap("a"));
	BOOST_CHECK(map.getMap("a.b"));
	BOOST_CHECK(map.getMap("a.b.c"));
	auto result = map.getMap("a.b.c");
	BOOST_REQUIRE(result);
	BOOST_CHECK_EQUAL(result->getString("d", ""), "value");
}
BOOST_AUTO_TEST_CASE(typeForcing) {
	Map map;
	map.set<bool>("value", 1);
	BOOST_CHECK_EQUAL(map.getBool("value", false), true);
}
BOOST_AUTO_TEST_CASE(stringTypes) {
	Map map;
	const char *constCharPointer = "value";
	auto charPointer = const_cast<char *>(constCharPointer);
	using CustomString = char *;
	auto customString = reinterpret_cast<CustomString>(charPointer);
	for (int i = 0; i < 2; i++) {
		map.set("a", constCharPointer);
		map.set("b", charPointer);
		map.set("c", customString);
		BOOST_CHECK_EQUAL(map.getString("a", ""), "value");
		BOOST_CHECK_EQUAL(map.getString("b", ""), "value");
		BOOST_CHECK_EQUAL(map.getString("c", ""), "value");
	}
}
BOOST_AUTO_TEST_CASE(spans) {
	Map map;
	bool bools[] = { true, true, false };
	int ints[] = { 0, 1, 2 };
	const char *strings[] = { "a", "b", "c" };
	map.set("bools", common::Span<bool>(bools, 3));
	map.set("ints", common::Span<int>(ints, 3));
	map.set("strings", common::Span<const char *>(strings, 3));
	BOOST_CHECK_EQUAL(map.getBools("bools").size(), 3u);
	BOOST_CHECK_EQUAL(map.getInt32s("ints").size(), 3u);
	BOOST_CHECK_EQUAL(map.getStrings("strings").size(), 3u);
}
static void fillTestData(Map &map) {
	map.set("bool", true);
	map.set("float", 0.5f);
	map.set("int32", 5);
	map.set("string", "a");
	map.set("boolArray", std::vector<bool>{ true, false, true });
	map.set("floatArray", std::vector<float>{ 1.0f, 2.0f, 3.0f });
	map.set("int32Array", std::vector<int32_t>{ 1, 2, 3 });
	map.set("stringArray", std::vector<std::string>{ "a", "b", "c" });
}
static void fillTypeMap(std::unordered_map<types::ValueType, uint8_t> &typeMap) {
	typeMap.emplace(types::ValueType::map, 1);
	typeMap.emplace(types::ValueType::basicBool, 2);
	typeMap.emplace(types::ValueType::basicFloat, 3);
	typeMap.emplace(types::ValueType::basicInt32, 4);
	typeMap.emplace(types::ValueType::color, 5);
	typeMap.emplace(types::ValueType::string, 6);
}
BOOST_AUTO_TEST_CASE(binarySerialize) {
	Map map;
	fillTestData(map);
	std::stringstream output(std::ios::out | std::ios::binary);
	std::unordered_map<types::ValueType, uint8_t> typeMap;
	fillTypeMap(typeMap);
	BOOST_REQUIRE(map.serialize(output, typeMap));
}
BOOST_AUTO_TEST_CASE(binaryDeserialize) {
	Map map;
	fillTestData(map);
	std::stringstream output(std::ios::out | std::ios::in | std::ios::binary);
	std::unordered_map<types::ValueType, uint8_t> typeMap;
	fillTypeMap(typeMap);
	BOOST_REQUIRE(map.serialize(output, typeMap));
	std::unordered_map<uint8_t, types::ValueType> valueTypeMap;
	for (auto i: typeMap) {
		valueTypeMap[i.second] = i.first;
	}
	Map resultMap;
	BOOST_REQUIRE(resultMap.deserialize(output, valueTypeMap));
	BOOST_CHECK_EQUAL(resultMap.size(), 4u);
}
BOOST_AUTO_TEST_SUITE_END()
