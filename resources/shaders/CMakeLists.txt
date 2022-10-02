macro(add_shader OUTPUT_NAME)

    set(FULL_OUT "${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT_NAME}")
    get_filename_component(OUT_DIR "${FULL_OUT}" DIRECTORY)
    file(MAKE_DIRECTORY "${OUT_DIR}")

    list(APPEND shaders_out "${FULL_OUT}")

    add_custom_command(OUTPUT ${FULL_OUT}
            DEPENDS ${ARGN} ${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE}
            COMMAND ${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE} -g --enhanced-msgs --target-env vulkan1.1 -t -o "${FULL_OUT}" ${ARGN}
            WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}"
            VERBATIM
    )
endmacro()

macro(add_radix_sort OUTPUT_PREFIX HASH GLOBALSORT)
    add_shader("${OUTPUT_PREFIX}_prescan.comp.spv" radixsort/prescan.comp ${HASH})
    add_shader("${OUTPUT_PREFIX}_globalsort.comp.spv" ${GLOBALSORT} radixsort/globalsort.comp ${HASH})
endmacro()

add_shader(test.vert.spv test.vert)
add_shader(test.frag.spv test.frag)

add_shader(particle.vert.spv particle.vert)
add_shader(particle.frag.spv particle.frag)

add_shader(neighbour/findcells.comp.spv neighbour/findcells.comp)
add_shader(neighbour/test.comp.spv neighbour/test.comp)

add_shader(radixsort/scan.comp.spv radixsort/scan.comp)
add_shader(radixsort/addblocksum.comp.spv radixsort/addblocksum.comp)

add_shader(selection/particle.vert.spv selection/particle.vert)
add_shader(selection/particle.frag.spv selection/particle.frag)

add_shader(simulation/unconstrainedupdate.comp.spv simulation/unconstrainedupdate.comp)
add_shader(simulation/keyinit.comp.spv simulation/keyinit.comp)
add_shader(simulation/particledataupdate.comp.spv simulation/particledataupdate.comp)

add_radix_sort(particlesort particlesort/hash.comp particlesort/globalsort.comp)
add_radix_sort(unittestsort unittestsort/hash.comp unittestsort/globalsort.comp)

add_custom_target(shaders-target DEPENDS "${shaders_out}")
