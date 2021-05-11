cmake_minimum_required(VERSION 3.10)
find_program(GIT_EXECUTABLE git DOC "Git version control")
mark_as_advanced(GIT_EXECUTABLE)
find_file(GITDIR NAMES .git PATHS ${CMAKE_CURRENT_SOURCE_DIR} NO_DEFAULT_PATH)
if (GIT_EXECUTABLE AND GITDIR)
	set(old_tz $ENV{TZ})
	set(ENV{TZ} UTC)
	execute_process(COMMAND "${GIT_EXECUTABLE}" describe "--match=gpick-*" "--match=v*" --always --long
		WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
		OUTPUT_VARIABLE version
		ERROR_VARIABLE version_error
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)
	if (version_error)
		message(FATAL_ERROR "Failed to get version: ${version_error}")
	endif()
	string(REGEX REPLACE "^(gpick-|v)([(0-9)]+\\.[(0-9)(a-z)]+(\\.[(0-9)(a-z)]+)?(-[(0-9)]+)?).*" "\\2" version_parts ${version})
	string(REPLACE "-" ";" version_parts ${version_parts})
	list(GET version_parts 0 GPICK_BUILD_VERSION)
	list(GET version_parts 1 GPICK_BUILD_REVISION)
	execute_process(COMMAND "${GIT_EXECUTABLE}" show --quiet "--date=format-local:%Y-%m-%d" "--format=%H;%cd"
		WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
		OUTPUT_VARIABLE version_parts
		ERROR_VARIABLE version_error
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)
	set(ENV{TZ} "${old_tz}")
	if (version_error)
		message(FATAL_ERROR "Failed to get version: ${version_error}")
	endif()
	list(LENGTH version_parts length)
	if (length LESS 2)
		message(FATAL_ERROR "Invalid version string ${version_parts}")
	endif()
	list(GET version_parts 0 GPICK_BUILD_HASH)
	list(GET version_parts 1 GPICK_BUILD_DATE)
	file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/.version" "${GPICK_BUILD_VERSION}\n${GPICK_BUILD_REVISION}\n${GPICK_BUILD_HASH}\n${GPICK_BUILD_DATE}")
else()
	find_file(VERSION_FILE NAMES .version PATHS ${CMAKE_CURRENT_SOURCE_DIR} NO_DEFAULT_PATH)
	if (NOT VERSION_FILE)
		message(FATAL_ERROR "Version file \".version\" is required when GIT can not be used to get version information.")
	endif()
	file(STRINGS "${VERSION_FILE}" version_parts LIMIT_COUNT 4)
	list(GET version_parts 0 GPICK_BUILD_VERSION)
	list(GET version_parts 1 GPICK_BUILD_REVISION)
	list(GET version_parts 2 GPICK_BUILD_HASH)
	list(GET version_parts 3 GPICK_BUILD_DATE)
endif()
set(GPICK_BUILD_VERSION_FULL "${GPICK_BUILD_VERSION}-${GPICK_BUILD_REVISION}")
string(REGEX REPLACE "([(0-9)]+)[^\\.]*" "\\1" version_part ${GPICK_BUILD_VERSION})
string(REPLACE "." "," GPICK_BUILD_VERSION_FULL_COMMA "${version_part},${GPICK_BUILD_REVISION}")
string(SUBSTRING ${GPICK_BUILD_HASH} 0 10 GPICK_BUILD_HASH)
