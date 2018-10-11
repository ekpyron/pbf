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
    std::string filename;

    vk::UniqueShaderModule realize(Context* context) const;

    bool operator<(const ShaderModule &rhs) const {
        return filename < rhs.filename;
    }
};

}

