find_package(Vulkan COMPONENTS glslang REQUIRED)

add_executable(computeShaderUnitTests
        ComputeShaderUnitTestContext.cpp ComputeShaderUnitTestContext.h ComputeShaderUnitTests.cpp
)
target_link_libraries(computeShaderUnitTests PUBLIC pbf_common Catch2::Catch2WithMain Vulkan::glslang)
add_dependencies(computeShaderUnitTests shaders-target)
