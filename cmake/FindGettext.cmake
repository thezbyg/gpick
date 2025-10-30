cmake_minimum_required(VERSION 3.22)
find_program(MSGMERGE_EXECUTABLE NAMES msgmerge)
find_program(MSGFMT_EXECUTABLE NAMES msgfmt)
find_program(MSGCAT_EXECUTABLE NAMES msgcat)
find_program(XGETTEXT_EXECUTABLE NAMES xgettext)
mark_as_advanced(MSGFMT_EXECUTABLE)
mark_as_advanced(MSGMERGE_EXECUTABLE)
mark_as_advanced(MSGCAT_EXECUTABLE)
mark_as_advanced(XGETTEXT_EXECUTABLE)
if (MSGFMT_EXECUTABLE)
	macro(MSGFMT_TARGET Name Input Output)
		set(MSGFMT_TARGET_usage "MSGFMT_TARGET(<Name> <Input> <Output> [COMPILE_FLAGS <string>]")
		if (${ARGC} GREATER 3)
			if (${ARGC} EQUAL 5)
				if ("${ARGV3}" STREQUAL "COMPILE_FLAGS")
					set(MSGFMT_EXECUTABLE_opts  "${ARGV4}")
					separate_arguments(MSGFMT_EXECUTABLE_opts)
				else()
					message(SEND_ERROR ${MSGFMT_TARGET_usage})
				endif()
			else()
				message(SEND_ERROR ${MSGFMT_TARGET_usage})
			endif()
		endif()
		get_filename_component(msgfmt_dir ${Output} DIRECTORY)
		file(MAKE_DIRECTORY ${msgfmt_dir})
		add_custom_command(
			OUTPUT ${Output}
			COMMAND ${MSGFMT_EXECUTABLE}
			ARGS ${MSGFMT_EXECUTABLE_opts} -f -o ${Output} ${Input}
			DEPENDS ${Input}
			COMMENT "Compiling translations ${Output}"
			WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		)
		set(MSGFMT_${Name}_DEFINED TRUE)
		set(MSGFMT_${Name}_OUTPUTS ${Output})
		set(MSGFMT_${Name}_INPUT ${Input})
		set(MSGFMT_${Name}_COMPILE_FLAGS ${MSGFMT_EXECUTABLE_opts})
	endmacro()
endif()
if (MSGCAT_EXECUTABLE)
	macro(MSGCAT_TARGET Name Input Output)
		set(MSGCAT_TARGET_usage "MSGCAT_TARGET(<Name> <Input> <Output> [COMPILE_FLAGS <string>]")
		if (${ARGC} GREATER 3)
			if (${ARGC} EQUAL 5)
				if ("${ARGV3}" STREQUAL "COMPILE_FLAGS")
					set(MSGCAT_EXECUTABLE_opts  "${ARGV4}")
					separate_arguments(MSGCAT_EXECUTABLE_opts)
				else()
					message(SEND_ERROR ${MSGCAT_TARGET_usage})
				endif()
			else()
				message(SEND_ERROR ${MSGCAT_TARGET_usage})
			endif()
		endif()
		get_filename_component(msgcat_dir ${Output} DIRECTORY)
		file(MAKE_DIRECTORY ${msgcat_dir})
		add_custom_command(
			OUTPUT ${Output}
			COMMAND ${MSGCAT_EXECUTABLE}
			ARGS ${MSGCAT_EXECUTABLE_opts} --output-file=${Output} ${Input}
			DEPENDS ${Input}
			COMMENT "Preparing translations ${Output}"
			WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		)
		set(MSGCAT_${Name}_DEFINED TRUE)
		set(MSGCAT_${Name}_OUTPUTS ${Output})
		set(MSGCAT_${Name}_INPUT ${Input})
		set(MSGCAT_${Name}_COMPILE_FLAGS ${MSGCAT_EXECUTABLE_opts})
	endmacro()
endif()
