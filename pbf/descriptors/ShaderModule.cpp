/**
 *
 *
 * @file ShaderModule.cpp
 * @brief 
 * @author clonker
 * @date 10/10/18
 */
#include "ShaderModule.h"

#include <pbf/Context.h>
#include <fstream>

using namespace pbf::descriptors;

vk::UniqueShaderModule ShaderModule::realize(Context *context) const {
    try {
        const auto &device = context->device();

        std::ifstream f;
        f.exceptions(std::ios_base::failbit|std::ios_base::badbit);
        f.open(filename);
        f.seekg(0, std::ios_base::end);
        auto length = static_cast<std::size_t>(f.tellg());
        f.seekg(0, std::ios_base::beg);

        std::vector<std::uint32_t> v;
        v.resize((length + 3) & ~3);

        f.read(reinterpret_cast<char *>(v.data()), length);

        return device.createShaderModuleUnique(vk::ShaderModuleCreateInfo({}, v.size(), v.data()));
    } catch (...) {
        std::throw_with_nested(std::runtime_error("Cannot read shader " + filename + "."));
    }
}
