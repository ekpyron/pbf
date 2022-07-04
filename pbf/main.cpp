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
#include "spdlog/sinks/stdout_color_sinks.h"

void print_exception(const std::exception &e, int level = 0)
{
    std::cerr << std::string(level, ' ') << "Exception: " << e.what() << std::endl;
    try {
        std::rethrow_if_nested(e);
    } catch(const std::exception &e) {
        print_exception(e, level + 1);
    } catch(...) {}
}

int main() {
    using namespace pbf;
    try {
        auto loggerConsole = spdlog::stdout_color_mt("console");
        loggerConsole->set_pattern("[%T.%f] %^[%l] %v%$");
#ifndef NDEBUG
        loggerConsole->set_level(spdlog::level::debug);
#endif
        auto loggerVulkan = spdlog::stdout_color_mt("vulkan");
        loggerVulkan->set_pattern("[%T.%f] %^[%l] [Vulkan] %v%$");
#ifndef NDEBUG
        loggerVulkan->set_level(spdlog::level::debug);
#endif

        {
            Context context;
            context.run();
        }
        loggerConsole->debug("Context destroyed. Returning from \"main\".");

        return 0;
    } catch (const std::exception& e) {
        print_exception(e, 0);
        return -1;
    }
}
