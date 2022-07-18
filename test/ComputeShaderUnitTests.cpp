#if defined(__clang__) && defined(clangd_feature_macro_workarounds)
#if __cpp_concepts != 201907U
#error "Weird"
#endif
#undef __cpp_concepts
#define __cpp_concepts 201907U
#undef __cpp_constexpr
#define __cpp_constexpr 201907U
#endif

#include <catch2/catch_test_macros.hpp>
#include "ComputeShaderUnitTestContext.h"
#include <ranges>

using ComputeShaderUnitTest = pbf::test::ComputeShaderUnitTestContext;

TEST_CASE_METHOD(ComputeShaderUnitTest, "Compute Shader Stub Test", "[stub]")
{
	using namespace pbf;

	auto stubLayout = cache().fetch(descriptors::DescriptorSetLayout{
		.createFlags = {},
		.bindings = {{
			.binding = 0,
			.descriptorType = vk::DescriptorType::eStorageBuffer,
			.descriptorCount = 1,
			.stageFlags = vk::ShaderStageFlagBits::eCompute
		}},
		PBF_DESC_DEBUG_NAME("Stub Descriptor Set Layout")
	});
	auto stubPipeline = cache().fetch(
		descriptors::ComputePipeline{
			.flags = {},
			.shaderStage = descriptors::ShaderStage {
				.stage = vk::ShaderStageFlagBits::eCompute,
				.module = cache().fetch(
					descriptors::ShaderModule{
						.source = compileComputeShader(R"(
#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_KHR_shader_subgroup_basic : enable

layout (local_size_x_id = 0, local_size_x = 2, local_size_y = 1, local_size_z = 1) in;

layout(std430, binding = 0) buffer Data
{
    uint data[];
};

layout(constant_id = 2) const uint shift = 0;

void main()
{
    data[gl_GlobalInvocationID.x] = (gl_GlobalInvocationID.x + shift) * 42;
}
						)"),
						PBF_DESC_DEBUG_NAME("shaders/stub.comp.spv Compute Shader")
					}),
				.entryPoint = "main",
				.specialization = {
					Specialization<uint32_t>{.constantID = 0, .value = uint32_t(4)},
					Specialization<uint32_t>{.constantID = 2, .value = uint32_t(7)}
				}
			},
			.pipelineLayout = cache().fetch(
				descriptors::PipelineLayout{
					.setLayouts = {{stubLayout}},
					PBF_DESC_DEBUG_NAME("prescan pipeline Layout")
				}),
			PBF_DESC_DEBUG_NAME("prescan pipeline")
		}
	);
	vk::UniqueDescriptorPool descriptorPool;
	{
		std::uint32_t numDescriptorSets = 1;
		static std::array<vk::DescriptorPoolSize, 1> sizes {{{ vk::DescriptorType::eStorageBuffer, 1 }}};
		descriptorPool = device().createDescriptorPoolUnique(vk::DescriptorPoolCreateInfo{
			.maxSets = numDescriptorSets,
			.poolSizeCount = static_cast<std::uint32_t>(sizes.size()),
			.pPoolSizes = sizes.data()
		});
	}

	auto stubParams = device().allocateDescriptorSets(vk::DescriptorSetAllocateInfo{
		.descriptorPool = *descriptorPool,
		.descriptorSetCount = 1,
		.pSetLayouts = &*stubLayout
	}).front();


	Buffer buffer{
		this,
		4 * 4 * sizeof(uint32_t),
		vk::BufferUsageFlagBits::eStorageBuffer,
		MemoryType::DYNAMIC
	};
	{
		auto descriptorInfos = {vk::DescriptorBufferInfo{buffer.buffer(), 0, buffer.size()}};
		device().updateDescriptorSets({vk::WriteDescriptorSet{
			.dstSet = stubParams,
			.dstBinding = 0,
			.dstArrayElement = 0,
			.descriptorCount = static_cast<uint32_t>(descriptorInfos.size()),
			.descriptorType = vk::DescriptorType::eStorageBuffer,
			.pImageInfo = nullptr,
			.pBufferInfo = &*descriptorInfos.begin(),
			.pTexelBufferView = nullptr
		}}, {});
	}

	run([&](vk::CommandBuffer buf) {
		buf.bindPipeline(vk::PipelineBindPoint::eCompute, *stubPipeline);
		buf.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *(stubPipeline.descriptor().pipelineLayout), 0, {stubParams}, {});
		buf.dispatch(4, 1, 1);
	});

	uint32_t *data = buffer.as<uint32_t>();
	for (size_t i = 0; i < 4*4; ++i)
		CHECK(data[i] == (i + 7) * 42);
}
