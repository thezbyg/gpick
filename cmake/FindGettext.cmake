cmake_minimum_required(VERSION 2.8)
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
		get_filename_component(dir ${Output} DIRECTORY)
		file(MAKE_DIRECTORY ${dir})
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
