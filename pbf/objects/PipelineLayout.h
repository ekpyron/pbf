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

    struct Descriptor {
        using Result = PipelineLayout;
    };

    const vk::PipelineLayout &get() const {
        return *_layout;
    }

    PipelineLayout(Context *context, const Descriptor &descriptor);

private:
    vk::UniquePipelineLayout _layout;
};

}
