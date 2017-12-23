set (textures_files font.png framing.png
        sky/skybox_negx.png sky/skybox_negy.png sky/skybox_negz.png
        sky/skybox_posx.png sky/skybox_posy.png sky/skybox_posz.png)

foreach(item IN ITEMS ${textures_files})
    list(APPEND textures_out "${CMAKE_CURRENT_BINARY_DIR}/${item}")
    add_custom_command(
            OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${item}"
            COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/${item}" "${CMAKE_CURRENT_BINARY_DIR}/${item}"
            DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${item}"
    )
endforeach()

message(STATUS "${textures_out}")

add_custom_target(textures-target DEPENDS "${textures_out}")
