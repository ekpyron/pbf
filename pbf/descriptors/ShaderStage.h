#pragma once

#include <pbf/common.h>
#include <pbf/Cache.h>
#include <pbf/descriptors/ShaderModule.h>
#include <concepts>

namespace pbf {

template<typename T>
concept SpecializationType = std::is_trivial_v<T> && std::is_standard_layout_v<T> && !std::is_pointer_v<T>;

template<SpecializationType T>
struct Specialization {
	uint32_t constantID;
	T value;
};
}

namespace pbf::descriptors
{

struct ShaderStage
{
	vk::ShaderStageFlagBits stage;
	CacheReference<ShaderModule> module;
	std::string entryPoint;

	struct SpecializationInfo
	{
		template<SpecializationType... Args>
		SpecializationInfo(Specialization<Args>... xs):
			SpecializationInfo(
				std::forward_as_tuple(xs.constantID...),
				std::forward_as_tuple(std::forward<Args>(xs.value)...),
				std::make_index_sequence<sizeof...(Args)>{}
			)
		{}
		auto operator<=>(SpecializationInfo const &) const = default;
		const std::vector<vk::SpecializationMapEntry>& mapEntries() const {
			return _mapEntries;
		}
		const void* data() const { return _data.data(); }
		size_t size() const { return _data.size(); }
	private:
		template<typename ConstantIDTuple, typename ValueTuple, size_t... I>
		SpecializationInfo(ConstantIDTuple&& constantIDs, ValueTuple&& values, std::index_sequence<I...>):
		_mapEntries{
			vk::SpecializationMapEntry{
				.constantID = std::get<I>(constantIDs),
				.offset = tupleElementSizes<ValueTuple>(std::make_index_sequence<I>{}),
				.size = sizeof(std::tuple_element_t<I, ValueTuple>)
			}...
		}
		{
			_data.resize(tupleElementSizes<ValueTuple>(std::index_sequence<I...>{}));
			(
				serialize(tupleElementSizes<ValueTuple>(std::make_index_sequence<I>{}), std::forward<std::tuple_element_t<I, ValueTuple>>(std::get<I>(values))), ...
			);
		}
		template<SpecializationType T>
		void serialize(uint32_t offset, T value) {
			*reinterpret_cast<T*>(&_data[offset]) = value;
		}
		template<typename TupleType, size_t... I>
		static constexpr uint32_t tupleElementSizes(std::index_sequence<I...>)
		{
			if constexpr (sizeof...(I) == 0) return 0;
			else return (sizeof(std::tuple_element_t<I, TupleType>) + ...);
		}
		std::vector<vk::SpecializationMapEntry> _mapEntries;
		std::vector<std::uint8_t> _data;
	};

	SpecializationInfo specialization;

	vk::PipelineShaderStageCreateInfo createInfo(vk::SpecializationInfo &specializationInfo) const
	{
		specializationInfo = vk::SpecializationInfo{
			.mapEntryCount = static_cast<uint32_t>(specialization.mapEntries().size()),
			.pMapEntries = specialization.mapEntries().data(),
			.dataSize = specialization.size(),
			.pData = specialization.data()
		};
		return vk::PipelineShaderStageCreateInfo{
			.stage = stage,
			.module = *module,
			.pName = entryPoint.c_str(),
			.pSpecializationInfo = &specializationInfo
		};
	}

private:
	using T = ShaderStage;
public:
	using Compare = PBFMemberComparator<&T::stage, &T::module, &T::entryPoint, &T::specialization>;
};

}