find_package (glfw3 REQUIRED)
find_package (PNG REQUIRED)
find_package (GLM REQUIRED)
find_package (OpenGL REQUIRED)
find_package (Threads REQUIRED)

include_directories (${GLFW_INCLUDE_DIR} ${CMAKE_SOURCE_DIR}/glcorew ${PNG_INCLUDE_DIRS} ${GLM_INCLUDE_DIR})

if (WIN32)
    set (CMAKE_EXE_LINKER_FLAGS "-static")
endif (WIN32)

file (GLOB PBF_SOURCES *.cpp)
set(PBF_SOURCES
        common.h
        Blur.h Blur.cpp
        Camera.h Camera.cpp
        Font.h Font.cpp
        Framing.h Framing.cpp
        FullscreenQuad.h FullscreenQuad.cpp
        NeighbourCellFinder.h NeighbourCellFinder.cpp
        PointSprite.h PointSprite.cpp
        RadixSort.h RadixSort.cpp
        Selection.h Selection.cpp
        ShaderProgram.h ShaderProgram.cpp
        Simulation.h Simulation.cpp
        Skybox.h Skybox.cpp
        SPH.h SPH.cpp
        SurfaceReconstruction.h SurfaceReconstruction.cpp
        Texture.h Texture.cpp)

add_library(pbf_lib ${PBF_SOURCES})
target_link_libraries(pbf_lib glfw ${PNG_LIBRARIES} ${OPENGL_LIBRARIES} glcorew spdlog Threads::Threads)

add_executable (pbf main.cpp)
target_link_libraries (pbf pbf_lib)
add_dependencies(pbf shaders-target textures-target)
