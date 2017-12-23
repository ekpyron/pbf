cmake_minimum_required (VERSION 3.5)
set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules")

project (pbf C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ensure that the build directory is not the source directory
include(MacroOutOfSourceBuild)
MACRO_ENSURE_OUT_OF_SOURCE_BUILD("${PROJECT_NAME} requires an out source build.")

add_subdirectory(libs/spdlog)

add_subdirectory (shaders)
add_subdirectory (textures)
add_subdirectory (src)
add_subdirectory (glcorew)

add_library(catch2 INTERFACE)
target_include_directories(catch2 INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/libs/Catch2/single_include")
add_subdirectory(test)

find_package (Doxygen QUIET)

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
