/**
 *
 *
 * @file ShaderModule.cpp
 * @brief 
 * @author clonker
 * @date 10/10/18
 */
#include "ShaderModule.h"

#include <pbf/VulkanContext.h>
#include <fstream>

using namespace pbf::descriptors;

template<typename... Args> struct LambdaVisitor : Args... { using Args::operator()...; };
template<typename... Args> LambdaVisitor(Args...) -> LambdaVisitor<Args...>;

pbf::ShaderModulePtr ShaderModule::realize(ContextInterface &context) const {
    const auto &device = context.device();
	auto result = std::make_unique<pbf::ShaderModule>();

	std::vector<uint32_t> spirvCodeFromFile;
    std::vector<uint32_t> const& spirvCode = std::visit(LambdaVisitor{
		[&](File const& _file) -> std::vector<uint32_t> const& {
			try {
				std::ifstream f;
				f.exceptions(std::ios_base::failbit|std::ios_base::badbit);
				f.open(_file.filename);
				f.seekg(0, std::ios_base::end);
				auto length = static_cast<std::size_t>(f.tellg());
				f.seekg(0, std::ios_base::beg);
				spirvCodeFromFile.resize((length + 3) & ~3);
				f.read(reinterpret_cast<char *>(spirvCodeFromFile.data()), length);
				return spirvCodeFromFile;
			} catch (...) {
				std::throw_with_nested(std::runtime_error("Cannot read shader " + _file.filename + "."));
			}
		},
		[&](RawSPIRV const& _raw) -> std::vector<uint32_t> const& {
			return _raw.content;
		}
	}, source);


	result->shaderModule = device.createShaderModuleUnique(vk::ShaderModuleCreateInfo({
		.codeSize = static_cast<uint32_t>(spirvCode.size()),
		.pCode = spirvCode.data()
	}));
	return result;
}
