find_package(GLSLANG REQUIRED)

macro(add_shader OUTPUT_NAME)

    set(FULL_OUT "${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT_NAME}")
    list(APPEND shaders_out "${FULL_OUT}")


    add_custom_command(OUTPUT ${FULL_OUT}
            DEPENDS ${ARGN} ${GLSLANG_VALIDATOR}
            COMMAND ${GLSLANG_VALIDATOR} --target-env vulkan1.1 -t -o ${FULL_OUT} ${ARGN}
            WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
            VERBATIM
    )
endmacro()

add_shader(test.vert.spv test.vert)
add_shader(test.frag.spv test.frag)

add_shader(triangle.vert.spv triangle.vert)
add_shader(triangle.frag.spv triangle.frag)

add_custom_target(shaders-target DEPENDS "${shaders_out}")
