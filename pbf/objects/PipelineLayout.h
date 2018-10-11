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

namespace pbf::objects {

class PipelineLayout {
public:
    vk::UniquePipelineLayout realize(Context *context) const;

    bool operator<(const PipelineLayout &rhs) const { return false; }
};

}
