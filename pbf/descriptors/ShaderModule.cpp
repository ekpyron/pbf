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

template<typename... Args> struct LambdaVisitor : Args... { using Args::operator()...; };
template<typename... Args> LambdaVisitor(Args...) -> LambdaVisitor<Args...>;

vk::UniqueShaderModule ShaderModule::realize(ContextInterface *context) const {
        const auto &device = context->device();


		return std::visit(LambdaVisitor{
			[&](File const& _file) {
				try {
					std::vector<std::uint32_t> v;
					std::ifstream f;
					f.exceptions(std::ios_base::failbit|std::ios_base::badbit);
					f.open(_file.filename);
					f.seekg(0, std::ios_base::end);
					auto length = static_cast<std::size_t>(f.tellg());
					f.seekg(0, std::ios_base::beg);

					v.resize((length + 3) & ~3);

					f.read(reinterpret_cast<char *>(v.data()), length);

					return device.createShaderModuleUnique(vk::ShaderModuleCreateInfo({
						  .codeSize = v.size(),
						  .pCode = v.data()
					}));
				} catch (...) {
					std::throw_with_nested(std::runtime_error("Cannot read shader " + _file.filename + "."));
				}
			},
			[&](RawSPIRV _raw) {
				return device.createShaderModuleUnique(vk::ShaderModuleCreateInfo({
				  .codeSize = static_cast<uint32_t>(_raw.content.size()) * sizeof(uint32_t),
				  .pCode = _raw.content.data()
			  }));
			}
		}, source);

}
