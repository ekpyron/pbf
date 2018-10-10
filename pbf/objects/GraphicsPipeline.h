/**
 *
 *
 * @file GraphicsPipeline.h
 * @brief 
 * @author clonker
 * @date 10/10/18
 */
#pragma once

#include <pbf/common.h>

namespace pbf::objects {

class GraphicsPipeline {
public:

    class Descriptor {
    public:
        using Result = GraphicsPipeline;
        Descriptor();
    };

    GraphicsPipeline();
};

}
