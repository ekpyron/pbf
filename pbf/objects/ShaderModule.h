/**
 *
 *
 * @file ShaderModule.h
 * @brief 
 * @author clonker
 * @date 10/10/18
 */
#pragma once

#include <pbf/common.h>

namespace pbf::objects {

class ShaderModule {
public:
    struct Descriptor {
        using Result = ShaderModule;

        std::string filename;
    };

    ShaderModule (Context* context, const Descriptor &descriptor);
private:
    vk::UniqueShaderModule _shaderModule;
};

}

