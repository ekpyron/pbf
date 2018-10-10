
# todo: include(FindPackageHandleStandardArgs) etc etc
find_program(GLSLANG_VALIDATOR glslangValidator)

if(NOT GLSLANG_VALIDATOR)
    message(FATAL_ERROR "Did not find glslang_validator")
endif()
