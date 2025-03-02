/**
 *
 *
 * @file ShaderModule.cpp
 * @brief 
 * @author clonker
 * @date 10/10/18
 */
#include "ShaderModule.h"

#include <spirv_reflect.h>
#include <pbf/VulkanContext.h>
#include <fstream>

using namespace pbf::descriptors;

template<typename... Args> struct LambdaVisitor : Args... { using Args::operator()...; };
template<typename... Args> LambdaVisitor(Args...) -> LambdaVisitor<Args...>;

pbf::ShaderModulePtr ShaderModule::realize(ContextInterface &context) const {
    const auto &device = context.device();
	auto shaderModule = std::make_unique<pbf::ShaderModule>();

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
				spirvCodeFromFile.resize(((length + 3) & ~3) / sizeof(uint32_t));
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

	SpvReflectShaderModule module = {};
	SpvReflectResult result = spvReflectCreateShaderModule2(SPV_REFLECT_MODULE_FLAG_NO_COPY, spirvCode.size() * sizeof(uint32_t), spirvCode.data(), &module);
	assert(result == SPV_REFLECT_RESULT_SUCCESS);

	uint32_t count = 0;
	result = spvReflectEnumerateDescriptorSets(&module, &count, NULL);
	assert(result == SPV_REFLECT_RESULT_SUCCESS);

	std::vector<SpvReflectDescriptorSet*> sets(count);
	result = spvReflectEnumerateDescriptorSets(&module, &count, sets.data());
	assert(result == SPV_REFLECT_RESULT_SUCCESS);

	spvReflectDestroyShaderModule(&module);

	shaderModule->shaderModule = device.createShaderModuleUnique(vk::ShaderModuleCreateInfo({
		.codeSize = static_cast<uint32_t>(spirvCode.size() * sizeof(uint32_t)),
		.pCode = spirvCode.data()
	}));
	return shaderModule;
}
