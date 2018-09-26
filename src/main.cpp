/**
 *
 *
 * @file main.cpp
 * @brief 
 * @author clonker
 * @date 9/26/18
 */

#include <iostream>
#include "Context.h"

int main() {
    using namespace pbf;
    try {
        auto logger = spdlog::stdout_color_mt("console");

        Context context;
        context.run();

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Failed: " << e.what() << std::endl;
        return -1;
    }
}
