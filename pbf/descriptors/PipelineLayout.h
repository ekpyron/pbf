/**
 *
 *
 * @file PipelineLayout.h
 * @brief 
 * @author clonker
 * @date 10/10/18
 */
#pragma once

#include <pbf/common.h>

namespace pbf::descriptors {

class PipelineLayout {
public:
    vk::UniquePipelineLayout realize(Context *context) const;

    struct Compare { bool operator()(...) const { return false; }};
};

}
