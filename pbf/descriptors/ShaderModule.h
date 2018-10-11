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
#include <pbf/descriptors/Order.h>

namespace pbf::descriptors {

struct ShaderModule {
    std::string filename;

    vk::UniqueShaderModule realize(Context* context) const;

    using Compare = MemberComparator<&ShaderModule::filename>;
};

}

