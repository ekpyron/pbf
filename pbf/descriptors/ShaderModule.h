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
#include "../ShaderModule.h"

namespace pbf::descriptors {

struct ShaderModule {
	struct File {
		std::string filename;
		using Compare = PBFMemberComparator<&File::filename>;
	};
	struct RawSPIRV {
		std::vector<std::uint32_t> content;
		using Compare = PBFMemberComparator<&RawSPIRV::content>;
	};
	std::variant<File, RawSPIRV> source;

    std::unique_ptr<pbf::ShaderModule> realize(ContextInterface &context) const;

    using Compare = PBFMemberComparator<&ShaderModule::source>;

#ifndef NDEBUG
    std::string debugName;
#endif
};

}

