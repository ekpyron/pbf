find_package (PNG REQUIRED)
find_package (GLM REQUIRED)
find_package(Java REQUIRED)
find_package(JNI REQUIRED)
find_package(GLEW REQUIRED)
find_package (OpenGL REQUIRED)
find_package (ASSIMP REQUIRED)

include_directories (${GLFW_INCLUDE_DIR} ${CMAKE_SOURCE_DIR}/glcorew ${PNG_INCLUDE_DIRS} ${GLM_INCLUDE_DIR} ${ASSIMP_INCLUDE_DIRS} ${JAVA_INCLUDE_PATH} ${JAVA_INCLUDE_PATH2})

include(UseJava)

set(CMAKE_JAVA_COMPILE_FLAGS "-source" "1.7" "-target" "1.7")

#if (WIN32)
#set (CMAKE_EXE_LINKER_FLAGS "-static")
#endif (WIN32)

file (GLOB PBF_SOURCES *.cpp)

add_library (pbf_jni MODULE ${PBF_SOURCES})

set_target_properties(pbf_jni PROPERTIES SUFFIX ".jnilib")
target_link_libraries(pbf_jni ${JNI_LIBRARIES} ${GLEW_LIBRARY} ${OPENGL_LIBRARY} ${PNG_LIBRARIES} ${ASSIMP_LIBRARIES})

set (USE_CXX11 YES CACHE BOOL "Enable C++11")
if (USE_CXX11)
set_property (TARGET pbf_jni PROPERTY COMPILE_FLAGS -std=c++11)
endif (USE_CXX11)
