set(shaders_files
        blur/fragment.glsl blur/vertex.glsl
        depthblur/vertex.glsl depthblur/fragment.glsl
        font/fragment.glsl font/vertex.glsl
        framing/fragment.glsl framing/vertex.glsl
        fsquad/fragment.glsl fsquad/vertex.glsl
        neighbourcellfinder/findcells.glsl neighbourcellfinder/neighbourcells.glsl
        noise/noise2D.glsl noise/noise3D.glsl
        particledepth/vertex.glsl particledepth/fragment.glsl
        particles/vertex.glsl particles/fragment.glsl
        radixsort/addblocksum.glsl radixsort/blockscan.glsl radixsort/counting.glsl radixsort/globalsort.glsl
        selection/fragment.glsl selection/vertex.glsl
        skybox/vertex.glsl skybox/fragment.glsl
        sph/calclambda.glsl sph/clearhighlight.glsl sph/highlight.glsl sph/predictpos.glsl
        sph/update.glsl sph/updatepos.glsl sph/vorticity.glsl
        thickness/fragment.glsl thickness/vertex.glsl)

foreach(item IN ITEMS ${shaders_files})
    list(APPEND shaders_out "${CMAKE_CURRENT_BINARY_DIR}/${item}")
    add_custom_command(
            OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${item}"
            COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/${item}" "${CMAKE_CURRENT_BINARY_DIR}/${item}"
            DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${item}"
    )
endforeach()

message(STATUS "${shaders_out}")

add_custom_target(shaders-target DEPENDS "${shaders_out}")
