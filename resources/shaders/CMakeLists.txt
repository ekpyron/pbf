macro(add_shader OUTPUT_NAME)

    set(FULL_OUT "${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT_NAME}")
    get_filename_component(OUT_DIR "${FULL_OUT}" DIRECTORY)
    file(MAKE_DIRECTORY "${OUT_DIR}")

    list(APPEND shaders_out "${FULL_OUT}")

    add_custom_command(OUTPUT ${FULL_OUT}
            DEPENDS ${ARGN} ${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE}
            COMMAND ${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE} --enhanced-msgs --target-env vulkan1.1 -t -o "${FULL_OUT}" ${ARGN}
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

add_shader(sort/prescan.comp.spv sort/scan_shared.comp sort/prescan.comp)
add_shader(sort/scan.comp.spv sort/scan_shared.comp sort/scan.comp)
add_shader(sort/assign.comp.spv sort/assign.comp)
add_shader(sort/addblocksum.comp.spv sort/addblocksum.comp)

#add_shader(radixsort/prescan.comp.spv radixsort/prescan.comp)
add_shader(radixsort/scan.comp.spv radixsort/scan.comp)
add_shader(radixsort/addblocksum.comp.spv radixsort/addblocksum.comp)
#add_shader(radixsort/globalsort.comp.spv radixsort/globalsort.comp)

add_radix_sort(particlesort particlesort/hash.comp particlesort/globalsort.comp)

add_custom_target(shaders-target DEPENDS "${shaders_out}")
