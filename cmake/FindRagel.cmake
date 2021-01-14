cmake_minimum_required(VERSION 2.8)
find_program(RAGEL_EXECUTABLE NAMES ragel DOC "path to the ragel executable")
mark_as_advanced(RAGEL_EXECUTABLE)
if (RAGEL_EXECUTABLE)
	execute_process(COMMAND ${RAGEL_EXECUTABLE} --version
		OUTPUT_VARIABLE RAGEL_version_output
		ERROR_VARIABLE RAGEL_version_error
		RESULT_VARIABLE RAGEL_version_result
		OUTPUT_STRIP_TRAILING_WHITESPACE)
	if (${RAGEL_version_result} EQUAL 0)
		string(REGEX REPLACE "^Ragel State Machine Compiler version ([^ ]+) .*$" "\\1" RAGEL_VERSION "${RAGEL_version_output}")
	else()
		message(SEND_ERROR "Command \"${RAGEL_EXECUTABLE} --version\" failed with output:
${RAGEL_version_error}")
	endif()
	macro(RAGEL_TARGET Name Input Output)
		set(RAGEL_TARGET_usage "RAGEL_TARGET(<Name> <Input> <Output> [COMPILE_FLAGS <string>]")
		if (${ARGC} GREATER 3)
			if (${ARGC} EQUAL 5)
				if ("${ARGV3}" STREQUAL "COMPILE_FLAGS")
					set(RAGEL_EXECUTABLE_opts  "${ARGV4}")
					separate_arguments(RAGEL_EXECUTABLE_opts)
				else()
					message(SEND_ERROR ${RAGEL_TARGET_usage})
				endif()
			else()
				message(SEND_ERROR ${RAGEL_TARGET_usage})
			endif()
		endif()
		add_custom_command(OUTPUT ${Output}
			COMMAND ${RAGEL_EXECUTABLE}
			ARGS ${RAGEL_EXECUTABLE_opts} -o${Output} ${Input}
			DEPENDS ${Input}
			COMMENT "Compiling state machine ${Output}"
			WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		)
		set(RAGEL_${Name}_DEFINED TRUE)
		set(RAGEL_${Name}_OUTPUTS ${Output})
		set(RAGEL_${Name}_INPUT ${Input})
		set(RAGEL_${Name}_COMPILE_FLAGS ${RAGEL_EXECUTABLE_opts})
	endmacro()
endif()
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Ragel REQUIRED_VARS RAGEL_EXECUTABLE VERSION_VAR RAGEL_VERSION)
