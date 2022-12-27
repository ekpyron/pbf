macro(add_shader_internal OUTPUT_NAME SOURCE_FILES DEPS ARGUMENTS)

    set(FULL_OUT "${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT_NAME}")
    get_filename_component(OUT_DIR "${FULL_OUT}" DIRECTORY)
    file(MAKE_DIRECTORY "${OUT_DIR}")

    list(APPEND shaders_out "${FULL_OUT}")

    add_custom_command(OUTPUT ${FULL_OUT}
            DEPENDS ${DEPS} ${Vulkan_GLSLC_EXECUTABLE}
            DEPFILE "${FULL_OUT}.d"
            COMMAND ${Vulkan_GLSLC_EXECUTABLE} -g -O --target-env=vulkan1.1 -MD -MF "${FULL_OUT}.d" -o "${FULL_OUT}" ${ARGUMENTS} ${SOURCE_FILES}
            WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}"
            VERBATIM
    )
endmacro()

macro(add_shader OUTPUT_NAME)
    add_shader_internal(${OUTPUT_NAME} "${ARGN}" "${ARGN}" "")
endmacro()

macro(add_radix_sort OUTPUT_PREFIX PARAMS_DIR)
    add_shader_internal("${OUTPUT_PREFIX}_prescan.comp.spv" "radixsort/prescan.comp" "radixsort/prescan.comp;${PARAMS_DIR}/params.comp" "-I;${PARAMS_DIR}")
    add_shader_internal("${OUTPUT_PREFIX}_globalsort.comp.spv" "radixsort/globalsort.comp" "radixsort/globalsort.comp;${PARAMS_DIR}/params.comp" "-I;${PARAMS_DIR}")
endmacro()

add_shader(test.vert.spv test.vert)
add_shader(test.frag.spv test.frag)

add_shader(particle.vert.spv particle.vert)
add_shader(particle.frag.spv particle.frag)

add_shader(neighbour/findcells.comp.spv neighbour/findcells.comp)

add_shader(radixsort/scan.comp.spv radixsort/scan.comp)
add_shader(radixsort/addblocksum.comp.spv radixsort/addblocksum.comp)

add_shader(selection/particle.vert.spv selection/particle.vert)
add_shader(selection/particle.frag.spv selection/particle.frag)

add_shader(simulation/unconstrainedupdate.comp.spv simulation/unconstrainedupdate.comp)
add_shader(simulation/keyinit.comp.spv simulation/keyinit.comp)
add_shader(simulation/particledataupdate.comp.spv simulation/particledataupdate.comp)
add_shader(simulation/calclambda.comp.spv simulation/calclambda.comp)
add_shader(simulation/updatepos.comp.spv simulation/updatepos.comp)
add_shader(simulation/calcvorticity.comp.spv simulation/calcvorticity.comp)
add_shader(simulation/updatevel.comp.spv simulation/updatevel.comp)

add_radix_sort(particlesort particlesort)
add_radix_sort(unittestsort unittestsort)

add_custom_target(shaders-target DEPENDS "${shaders_out}")
