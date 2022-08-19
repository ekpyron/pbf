#include <catch2/catch_test_macros.hpp>

#include <pbf/VulkanObjectType.h>

struct FakeHandle {
    static constexpr int objectType = 5;
};

TEST_CASE("Known Vulkan Object Test", "[vulkan object]") {
    if constexpr (pbf::KnownVulkanObject<vk::CommandBuffer>) {
        SUCCEED();
    } else {
        FAIL();
    }

    if constexpr (pbf::KnownVulkanObject<std::string>) {
        FAIL();
    } else {
        SUCCEED();
    }

    if constexpr (pbf::KnownVulkanObject<FakeHandle>) {
        FAIL();
    } else {
        SUCCEED();
    }
}
