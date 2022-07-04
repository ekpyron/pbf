#pragma once

#include <pbf/common.h>
#include <pbf/Cache.h>
#include <pbf/descriptors/ShaderModule.h>

namespace pbf::descriptors {
struct ShaderStage {
	vk::ShaderStageFlagBits stage;
	CacheReference<ShaderModule> module;
	std::string entryPoint;

	vk::PipelineShaderStageCreateInfo createInfo() const {
		return vk::PipelineShaderStageCreateInfo{
			.stage = stage,
			.module = *module,
			.pName = entryPoint.c_str(),
			.pSpecializationInfo = nullptr
		};
	}

	template<typename T = ShaderStage>
	using Compare = PBFMemberComparator<&T::stage, &T::module, &T::entryPoint>;
};
}