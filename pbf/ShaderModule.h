#pragma once
#include <pbf/common.h>

namespace pbf
{

struct ShaderModule {
    vk::UniqueShaderModule shaderModule;

    bool operator!() const { return !shaderModule; }
};

}
