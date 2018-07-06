#include <boost/test/unit_test.hpp>
#include <fstream>
#include <iostream>
#include "dynv/DynvSystem.h"
#include "dynv/DynvXml.h"
#include "dynv/DynvVarString.h"
#include "dynv/DynvVarInt32.h"
#include "dynv/DynvVarColor.h"
#include "dynv/DynvVarFloat.h"
#include "dynv/DynvVarDynv.h"
#include "dynv/DynvVarBool.h"
#include "dynv/DynvVarPtr.h"
using namespace std;

static dynvSystem* buildDynv()
{
	auto handler_map = dynv_handler_map_create();
	dynv_handler_map_add_handler(handler_map, dynv_var_string_new());
	dynv_handler_map_add_handler(handler_map, dynv_var_int32_new());
	dynv_handler_map_add_handler(handler_map, dynv_var_color_new());
	dynv_handler_map_add_handler(handler_map, dynv_var_ptr_new());
	dynv_handler_map_add_handler(handler_map, dynv_var_float_new());
	dynv_handler_map_add_handler(handler_map, dynv_var_dynv_new());
	dynv_handler_map_add_handler(handler_map, dynv_var_bool_new());
	auto dynv = dynv_system_create(handler_map);
	dynv_handler_map_release(handler_map);
	return dynv;
}
BOOST_AUTO_TEST_CASE(xml_deserialization)
{
	auto dynv = buildDynv();
	ifstream file("test/config01.xml");
	BOOST_REQUIRE(file.is_open());
	if (file.is_open()){
		BOOST_REQUIRE(dynv_xml_deserialize(dynv, file) == 0);
		file.close();
	}
	const char *data[] = {"a", "b", "c"};
	int error;
	uint32_t count;
	char** values = (char**)dynv_get_array(dynv, "string", "test", &count, &error);
	BOOST_CHECK(error == 0);
	BOOST_CHECK(values != nullptr);
	BOOST_CHECK(count == 3);
	for (int i = 0; i < 3; i++){
		string value(data[i]);
		BOOST_CHECK(value == values[i]);
	}
	delete [] values;
	BOOST_CHECK(dynv_system_release(dynv) == 0);
}
BOOST_AUTO_TEST_CASE(string_array)
{
	auto dynv = buildDynv();
	const char *data[] = {"a", "b", "c"};
	dynv_set_array(dynv, "string", "a", (const void**)data, 3);
	int error;
	uint32_t count;
	char** values = (char**)dynv_get_array(dynv, "string", "a", &count, &error);
	BOOST_CHECK(error == 0);
	BOOST_CHECK(values != nullptr);
	BOOST_CHECK(count == 3);
	for (int i = 0; i < 3; i++){
		string value(data[i]);
		BOOST_CHECK(value == values[i]);
	}
	delete [] values;
	BOOST_CHECK(dynv_system_release(dynv) == 0);
}
BOOST_AUTO_TEST_CASE(string_array_overwrite)
{
	auto dynv = buildDynv();
	const char *data[] = {"a", "b", "c"};
	dynv_set_array(dynv, "string", "a", (const void**)data, 3);
	dynv_set_array(dynv, "string", "a", (const void**)data, 3);
	int error;
	uint32_t count;
	char** values = (char**)dynv_get_array(dynv, "string", "a", &count, &error);
	BOOST_CHECK(error == 0);
	BOOST_CHECK(values != nullptr);
	BOOST_CHECK(count == 3);
	for (int i = 0; i < 3; i++){
		string value(data[i]);
		BOOST_CHECK(value == values[i]);
	}
	delete [] values;
	BOOST_CHECK(dynv_system_release(dynv) == 0);
}
BOOST_AUTO_TEST_CASE(dynv_array)
{
	auto dynv = buildDynv();
	dynvSystem *data[] = {dynv_system_create(dynv), dynv_system_create(dynv), dynv_system_create(dynv)};
	dynv_set_array(dynv, "dynv", "a", (const void**)data, 3);
	int error;
	uint32_t count;
	dynvSystem** values = (dynvSystem**)dynv_get_array(dynv, "dynv", "a", &count, &error);
	BOOST_CHECK(error == 0);
	BOOST_CHECK(values != nullptr);
	BOOST_CHECK(count == 3);
	for (int i = 0; i < 3; i++){
		BOOST_CHECK(values[i] == data[i]);
		BOOST_CHECK(dynv_system_release(values[i]) == -1);
	}
	for (int i = 0; i < 3; i++){
		BOOST_CHECK(dynv_system_release(data[i]) == -1);
	}
	delete [] values;
	BOOST_CHECK(dynv_system_release(dynv) == 0);
}
BOOST_AUTO_TEST_CASE(dynv_array_null)
{
	auto dynv = buildDynv();
	dynvSystem *data[] = {dynv_system_create(dynv), dynv_system_create(dynv), dynv_system_create(dynv)};
	dynv_set_array(dynv, "dynv", "a", (const void**)data, 3);
	dynv_set_array(dynv, "dynv", "a", nullptr, 0);
	int error;
	uint32_t count;
	dynvSystem** values = (dynvSystem**)dynv_get_array(dynv, "dynv", "a", &count, &error);
	BOOST_CHECK(error != 0);
	BOOST_CHECK(values == nullptr);
	BOOST_CHECK(count == 0);
	for (int i = 0; i < 3; i++){
		BOOST_CHECK(dynv_system_release(data[i]) == 0);
	}
	delete [] values;
	BOOST_CHECK(dynv_system_release(dynv) == 0);
}
