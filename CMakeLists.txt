cmake_minimum_required (VERSION 2.8)
set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules")

project (pbf)

add_subdirectory (src)
add_subdirectory (glcorew)

find_package (ASSIMP REQUIRED)
find_package (Doxygen QUIET)

include_directories (${ASSIMP_INCLUDE_DIRS})
target_link_libraries (${ASSIMP_LIBRARIES})

if (DOXYGEN_FOUND)
	find_package (LATEX QUIET)
	find_program (DOXYFILE_MAKE make)
	mark_as_advanced(DOXYFILE_MAKE)
	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
	add_custom_target(doc ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
						WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
						COMMENT "Generating API documentation with Doxygen" VERBATIM)	
	if(LATEX_COMPILER AND MAKEINDEX_COMPILER AND DOXYFILE_MAKE)
			add_custom_command(TARGET doc
				POST_BUILD
				COMMAND "${DOXYFILE_MAKE}"
				COMMENT	"Running LaTeX for Doxygen documentation in ${CMAKE_CURRENT_BINARY_DIR}/latex..."
				WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/latex")
	endif()
endif (DOXYGEN_FOUND)
