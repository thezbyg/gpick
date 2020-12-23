cmake_minimum_required(VERSION 3.1)
project(gpick)
option(ENABLE_NLS "compile with gettext support" true)
option(USE_GTK3 "use GTK3 instead of GTK2" true)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
file(GLOB SOURCES
	source/*.cpp source/*.h
	source/color_names/*.cpp source/color_names/*.h
	source/dbus/*.cpp source/dbus/*.h source/dbus/*.c
	source/gtk/*.cpp source/gtk/*.h
	source/i18n/*.cpp source/i18n/*.h
	source/layout/*.cpp source/layout/*.h
	source/lua/*.cpp source/lua/*.h
	source/tools/*.cpp source/tools/*.h
	source/transformation/*.cpp source/transformation/*.h
)
list(REMOVE_ITEM SOURCES source/Color.cpp source/Color.h source/MathUtil.cpp source/MathUtil.h source/lua/Script.cpp source/lua/Script.h)
include(Version)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/source/version/Version.cpp.in" "${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/Version.cpp" @ONLY)
list(APPEND SOURCES "${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/Version.cpp")
find_package(Boost 1.58 COMPONENTS filesystem system unit_test_framework REQUIRED)
find_package(PkgConfig)
if (PkgConfig_FOUND)
	if (USE_GTK3)
		pkg_check_modules(GTK3 gtk+-3.0>=3.0)
	else()
		pkg_check_modules(GTK2 gtk+-2.0>=2.24)
		pkg_check_modules(GioUnix gio-unix-2.0>=2.24)
	endif()
	pkg_search_module(Lua lua5.4>=5.4 lua5>=5.4 lua5.3>=5.3 lua5>=5.3 lua>=5.3 lua5.2>=5.2 lua>=5.2)
	pkg_check_modules(Expat expat>=1.0)
endif (PkgConfig_FOUND)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
find_package(Ragel 6.9 REQUIRED)

function(set_compile_options target)
	if (MSVC)
		target_compile_options(${target} PRIVATE /MD /utf-8)
		target_compile_options(${target} PRIVATE $<$<COMPILE_LANGUAGE:C>:/TP>)
		target_compile_definitions(${target} PRIVATE UNICODE GSEAL_ENABLE)
		if (ENABLE_NLS)
			target_compile_definitions(${target} PRIVATE ENABLE_NLS)
		endif()
	else()
		target_compile_options(${target} PRIVATE -Wall -Wfloat-conversion)
		target_compile_options(${target} PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-std=c++14>)
		if (ENABLE_NLS)
			target_compile_definitions(${target} PRIVATE ENABLE_NLS "LOCALEDIR=${CMAKE_INSTALL_PREFIX}/share/locale")
		endif()
	endif()
endfunction()

function(add_gtk_options target)
	if (USE_GTK3)
		target_link_libraries(${target} PRIVATE
			${GTK3_LIBRARIES}
		)
		target_include_directories(${target} PRIVATE
			${GTK3_INCLUDE_DIRS}
		)
	else()
		target_link_libraries(${target} PRIVATE
			${GTK2_LIBRARIES}
			${GioUnix_LIBRARIES}
		)
		target_include_directories(${target} PRIVATE
			${GTK2_INCLUDE_DIRS}
			${GioUnix_INCLUDE_DIRS}
		)
	endif()
endfunction()

file(GLOB MATH_SOURCES source/MathUtil.cpp source/MathUtil.h)
add_library(gpick-math ${MATH_SOURCES})
set_compile_options(gpick-math)
target_include_directories(gpick-math PRIVATE
	source
	${Boost_INCLUDE_DIRS}
)

file(GLOB COLOR_SOURCES source/Color.cpp source/Color.h)
add_library(gpick-color ${COLOR_SOURCES})
set_compile_options(gpick-color)
target_link_libraries(gpick-color PRIVATE gpick-math)
target_include_directories(gpick-color PRIVATE
	source
	${Boost_INCLUDE_DIRS}
)

file(GLOB COMMON_SOURCES source/common/*.cpp source/common/*.h)
add_library(gpick-common ${COMMON_SOURCES})
set_compile_options(gpick-common)
target_include_directories(gpick-common PRIVATE
	source
	${Boost_INCLUDE_DIRS}
)

file(GLOB DYNV_SOURCES source/dynv/*.cpp source/dynv/*.h)
add_library(gpick-dynv ${DYNV_SOURCES})
set_compile_options(gpick-dynv)
target_link_libraries(gpick-dynv PRIVATE gpick-color)
target_include_directories(gpick-dynv PRIVATE
	source
	${Expat_INCLUDE_DIRS}
	${Boost_INCLUDE_DIRS}
)

file(GLOB LUA_SOURCES source/lua/Script.cpp source/lua/Script.h)
add_library(gpick-lua ${LUA_SOURCES})
set_compile_options(gpick-lua)
target_link_libraries(gpick-lua PRIVATE
	${Lua_LIBRARIES}
)
target_include_directories(gpick-lua PRIVATE
	source
	${Lua_INCLUDE_DIRS}
	${Boost_INCLUDE_DIRS}
)

file(GLOB PARSER_SOURCES source/parser/*.cpp source/parser/*.h)
ragel_target(text_file_parser source/parser/TextFileParser.rl ${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/TextFileParser.cpp)
list(APPEND PARSER_SOURCES ${RAGEL_text_file_parser_OUTPUTS})
add_library(gpick-parser ${PARSER_SOURCES})
set_compile_options(gpick-parser)
target_include_directories(gpick-parser PRIVATE
	source
	${Boost_INCLUDE_DIRS}
)

if (ENABLE_NLS)
	find_package(Gettext REQUIRED)
	file(GLOB TRANSLATIONS share/locale/*/LC_MESSAGES/gpick.po)
	foreach(translation ${TRANSLATIONS})
		file(RELATIVE_PATH name "${CMAKE_CURRENT_SOURCE_DIR}" ${translation})
		get_filename_component(dir ${name} DIRECTORY)
		msgfmt_target(translation ${translation} ${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/${dir}/gpick.mo)
		set(MSGFMT_translation_COMPILE_FLAGS "--check-format --check-domain")
		list(APPEND TRANSLATION_FILES ${MSGFMT_translation_OUTPUTS})
	endforeach()
	add_custom_target(translations ALL DEPENDS ${TRANSLATION_FILES})
	file(GLOB LUA_SOURCES share/gpick/*.lua)
	add_custom_target(template
		COMMAND ${XGETTEXT_EXECUTABLE} --keyword=_ --keyword=N_ --from-code=UTF-8 --package-name=gpick --package-version=0.0 --output=${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/template_c.pot ${SOURCES}
		COMMAND ${XGETTEXT_EXECUTABLE} --language=C++ --keyword=_ --keyword=N_ --from-code=UTF-8 --package-name=gpick --package-version=0.0 --output=${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/template_lua.pot ${LUA_SOURCES}
		COMMAND ${MSGCAT_EXECUTABLE} --use-first ${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/template_c.pot ${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/template_lua.pot --output-file=${CMAKE_CURRENT_BINARY_DIR}/template.pot
	)
endif()

add_executable(gpick ${SOURCES})
set_compile_options(gpick)
add_gtk_options(gpick)
target_link_libraries(gpick PRIVATE
	gpick-color
	gpick-math
	gpick-dynv
	gpick-lua
	gpick-parser
	gpick-common
	${Boost_FILESYSTEM_LIBRARY}
	${Boost_SYSTEM_LIBRARY}
	${Lua_LIBRARIES}
	${Expat_LIBRARIES}
	Threads::Threads
)
target_include_directories(gpick PRIVATE
	source
	${Boost_INCLUDE_DIRS}
	${Lua_INCLUDE_DIRS}
	${Expat_INCLUDE_DIRS}
)

file(GLOB TESTS_SOURCES source/test/*.cpp source/test/*.h)
add_executable(tests ${TESTS_SOURCES})
set_compile_options(tests)
target_compile_definitions(tests PRIVATE BOOST_TEST_DYN_LINK)
target_link_libraries(tests PRIVATE
	gpick-color
	gpick-math
	gpick-dynv
	gpick-lua
	gpick-parser
	gpick-common
	${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
	${Lua_LIBRARIES}
	${Expat_LIBRARIES}
	Threads::Threads
)
target_include_directories(tests PRIVATE
	source
	${Boost_INCLUDE_DIRS}
	${Lua_INCLUDE_DIRS}
	${Expat_INCLUDE_DIRS}
)

install(TARGETS gpick DESTINATION bin)
install(FILES share/metainfo/gpick.appdata.xml DESTINATION share/metainfo)
install(FILES share/applications/gpick.desktop DESTINATION share/applications)
install(FILES share/mime/packages/gpick.xml DESTINATION share/mime/packages)
install(FILES share/doc/gpick/copyright DESTINATION share/doc/gpick)
install(FILES share/man/man1/gpick.1 DESTINATION share/man/man1)
file(GLOB RESOURCE_FILES share/gpick/*.png share/gpick/*.lua share/gpick/*.txt)
install(FILES ${RESOURCE_FILES} DESTINATION share/gpick)
install(DIRECTORY share/icons DESTINATION share)
if (ENABLE_NLS)
	foreach(translation ${TRANSLATION_FILES})
		file(RELATIVE_PATH name "${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}" ${translation})
		get_filename_component(dir ${name} DIRECTORY)
		install(FILES ${translation} DESTINATION ${dir})
	endforeach()
endif()
