/**
 *
 *
 * @file ShaderModule.h
 * @brief 
 * @author clonker
 * @date 10/10/18
 */
#pragma once

#include <pbf/common.h>
#include <pbf/descriptors/Order.h>
#include <variant>

namespace pbf::descriptors {

struct ShaderModule {
	struct File {
		std::string filename;
		bool operator<(const File& _rhs) const { return filename < _rhs.filename; }
	};
	struct RawSPIRV {
		std::vector<std::uint32_t> content;
		bool operator<(const RawSPIRV& _rhs) const {  return content < _rhs.content; }
	};
	std::variant<File, RawSPIRV> source;

    vk::UniqueShaderModule realize(ContextInterface* context) const;

    using Compare = PBFMemberComparator<&ShaderModule::source>;

#ifndef NDEBUG
    std::string debugName;
#endif
};

}

