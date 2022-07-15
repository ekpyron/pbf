macro(add_shader OUTPUT_NAME)

    set(FULL_OUT "${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT_NAME}")
    list(APPEND shaders_out "${FULL_OUT}")


    add_custom_command(OUTPUT ${FULL_OUT}
            DEPENDS ${ARGN} ${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE}
            COMMAND ${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE} --target-env vulkan1.1 -t -o ${FULL_OUT} ${ARGN}
            WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
            VERBATIM
    )
endmacro()

add_shader(test.vert.spv test.vert)
add_shader(test.frag.spv test.frag)

add_shader(particle.vert.spv particle.vert)
add_shader(particle.frag.spv particle.frag)

add_shader(sort/prescan.comp.spv sort/prescan.comp)
add_shader(sort/scan.comp.spv sort/scan.comp)

add_custom_target(shaders-target DEPENDS "${shaders_out}")
