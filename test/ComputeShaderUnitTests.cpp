#if defined(__clang__) && defined(clangd_feature_macro_workarounds)
#if __cpp_concepts != 201907U
//#error "Weird"
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
					Specialization<uint32_t>{.constantID = 0, .value = 4},
					Specialization<uint32_t>{.constantID = 2, .value = 7}
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


TEST_CASE_METHOD(ComputeShaderUnitTest, "Compute Shader Sort Test", "[sort]")
{
	using namespace pbf;

	std::vector<uint32_t> initialKeys{
		100, 1, 14, 13, 4, 5, 6, 7, 300, 9, 10, 11, 12, 3, 2, 15,
		100, 1, 142314, 13, 4, 5, 6, 7, 421269, 9, 10, 11, 12, 3, 2, 15
	};
	std::reverse(initialKeys.begin(), initialKeys.end());
	uint32_t numKeys = static_cast<uint32_t>(initialKeys.size());
	const uint32_t blockSize = 16; // apparently has to be >= initialKeys.size() / 2 right now

	device().waitIdle();
	Buffer keys{
		this,
		numKeys * sizeof(uint32_t),
		vk::BufferUsageFlagBits::eStorageBuffer|vk::BufferUsageFlagBits::eTransferDst,
		MemoryType::DYNAMIC
	};
	{
		uint32_t *data = keys.as<uint32_t>();
		for (uint32_t i = 0; i < numKeys; ++i)
			data[i] = initialKeys[i];
	}
	keys.flush();
	device().waitIdle();
	Buffer result{
		this,
		numKeys * sizeof(uint32_t),
		vk::BufferUsageFlagBits::eStorageBuffer|vk::BufferUsageFlagBits::eTransferSrc,
		MemoryType::STATIC
	};

	Buffer prefixSums{
		this,
		numKeys * sizeof(uint32_t),
		vk::BufferUsageFlagBits::eStorageBuffer,
		MemoryType::STATIC
	};

	uint32_t numGroups = (numKeys + blockSize - 1) / blockSize;

	Buffer blockSums{
		this,
		4 * numGroups * sizeof(uint32_t),
		vk::BufferUsageFlagBits::eStorageBuffer,
		MemoryType::STATIC
	};
	Buffer blockSums2{
		this,
		4 * numGroups * sizeof(uint32_t), // TODO
		vk::BufferUsageFlagBits::eStorageBuffer,
		MemoryType::STATIC
	};

	auto sortLayout = cache().fetch(descriptors::DescriptorSetLayout{
		.createFlags = {},
		.bindings = {{
			.binding = 0,
			.descriptorType = vk::DescriptorType::eStorageBuffer,
			.descriptorCount = 1,
			.stageFlags = vk::ShaderStageFlagBits::eCompute
		},{
			.binding = 1,
			.descriptorType = vk::DescriptorType::eStorageBuffer,
			.descriptorCount = 1,
			.stageFlags = vk::ShaderStageFlagBits::eCompute
		},{
			.binding = 2,
			.descriptorType = vk::DescriptorType::eStorageBuffer,
			.descriptorCount = 1,
			.stageFlags = vk::ShaderStageFlagBits::eCompute
		},{
			.binding = 3,
			.descriptorType = vk::DescriptorType::eStorageBuffer,
			.descriptorCount = 1,
			.stageFlags = vk::ShaderStageFlagBits::eCompute
		}},
		PBF_DESC_DEBUG_NAME("Stub Descriptor Set Layout")
	});
	auto sortPipelineLayout = cache().fetch(
	descriptors::PipelineLayout{
		.setLayouts = {{sortLayout}},
		.pushConstants = {
			vk::PushConstantRange{
				vk::ShaderStageFlagBits::eCompute,
				0,
				sizeof(uint32_t) * 4 +
				sizeof(uint32_t)
			}
		},
		PBF_DESC_DEBUG_NAME("prescan pipeline Layout")
	});
	auto prescanPipeline = cache().fetch(
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

layout (local_size_x_id = 0, local_size_y = 1, local_size_z = 1) in;

const uint HALFBLOCKSIZE = gl_WorkGroupSize.x;
const uint BLOCKSIZE = 2 * HALFBLOCKSIZE;


layout(std430, binding = 0) readonly buffer Keys
{
    uint keys[];
};

layout(std430, binding = 1) writeonly buffer PrefixSums
{
    uint prefixSum[];
};

layout(std430, binding = 2) writeonly buffer BlockSums
{
    uint blockSum[];
};

uint GetHash(uint id) {
	return keys[id];
}

layout(push_constant) uniform constants {
	uvec4 blockSumOffsets;
    int bitShift;
};

shared uvec4 tmp[BLOCKSIZE];

// TODO: what *exactly* is needed :-)?
//#define BARRIER() barrier(); memoryBarrier(); memoryBarrierShared(); groupMemoryBarrier(); barrier()
#define BARRIER() memoryBarrier(); groupMemoryBarrier(); barrier()

void main()
{
	uint bits1 = bitfieldExtract(GetHash(gl_WorkGroupID.x * BLOCKSIZE + gl_LocalInvocationIndex), bitShift, 2);
	uint bits2 = bitfieldExtract(GetHash(gl_WorkGroupID.x * BLOCKSIZE + gl_LocalInvocationIndex + HALFBLOCKSIZE), bitShift, 2);

	tmp[gl_LocalInvocationIndex] = uvec4(equal(bits1 * uvec4(1,1,1,1), uvec4(0,1,2,3)));
	tmp[gl_LocalInvocationIndex + HALFBLOCKSIZE] = uvec4(equal(bits2 * uvec4(1,1,1,1), uvec4(0,1,2,3)));


    int offset = 1;
    for (uint d = HALFBLOCKSIZE; d > 0; d >>= 1)
    {
        BARRIER();

        if (gl_LocalInvocationIndex < d) {
            uint i = offset * (2 * gl_LocalInvocationIndex + 1) - 1;
            uint j = offset * (2 * gl_LocalInvocationIndex + 2) - 1;
            tmp[j] += tmp[i];
        }
        offset <<= 1;
    }

	BARRIER();

	if (gl_LocalInvocationIndex == 0)
	{
		for(uint i = 0; i < 4; ++i)
			blockSum[blockSumOffsets[i] + gl_WorkGroupID.x] = uvec4(tmp[BLOCKSIZE - 1])[i];
		tmp[BLOCKSIZE - 1] = uvec4(0, 0, 0, 0);
	}

    for (uint d = 1; d < BLOCKSIZE; d <<= 1)
    {
        offset >>= 1;
        BARRIER();

        if (gl_LocalInvocationIndex < d)
        {
            uint i = offset * (2 * gl_LocalInvocationIndex + 1) - 1;
            uint j = offset * (2 * gl_LocalInvocationIndex + 2) - 1;
            uvec4 t = tmp[i];
            tmp[i] = tmp[j];
            tmp[j] += t;
        }
    }

    BARRIER();

    prefixSum[gl_WorkGroupID.x * BLOCKSIZE + gl_LocalInvocationIndex] = uvec4(tmp[gl_LocalInvocationIndex])[bits1];
    prefixSum[gl_WorkGroupID.x * BLOCKSIZE + gl_LocalInvocationIndex + HALFBLOCKSIZE] = uvec4(tmp[gl_LocalInvocationIndex + HALFBLOCKSIZE])[bits2];
}
						)"),
						PBF_DESC_DEBUG_NAME("Sort Compute Shader")
					}),
				.entryPoint = "main",
				.specialization = {
					Specialization<uint32_t>{.constantID = 0, .value = blockSize / 2 }
				}
			},
			.pipelineLayout = sortPipelineLayout,
			PBF_DESC_DEBUG_NAME("prescan pipeline")
		}
	);
	auto scanPipeline = cache().fetch(
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

layout (local_size_x_id = 0, local_size_y = 1, local_size_z = 1) in;

const uint HALFBLOCKSIZE = gl_WorkGroupSize.x;
const uint BLOCKSIZE = 2 * HALFBLOCKSIZE;

// TODO: what *exactly* is needed :-)?
//#define BARRIER() barrier(); memoryBarrier(); memoryBarrierShared(); groupMemoryBarrier(); barrier()
#define BARRIER() memoryBarrier(); groupMemoryBarrier(); barrier()

layout(std430, binding = 1) buffer PrefixSums
{
    uint prefixSum[];
};

layout(std430, binding = 2) buffer BlockSums
{
    uint blockSum[];
};

shared uint tmp[BLOCKSIZE];

void main()
{
	tmp[gl_LocalInvocationIndex] = prefixSum[gl_WorkGroupID.x * BLOCKSIZE + gl_LocalInvocationIndex];
	tmp[gl_LocalInvocationIndex + HALFBLOCKSIZE] = prefixSum[gl_WorkGroupID.x * BLOCKSIZE + gl_LocalInvocationIndex + HALFBLOCKSIZE];


    int offset = 1;
    for (uint d = HALFBLOCKSIZE; d > 0; d >>= 1)
    {
        BARRIER();

        if (gl_LocalInvocationIndex < d) {
            uint i = offset * (2 * gl_LocalInvocationIndex + 1) - 1;
            uint j = offset * (2 * gl_LocalInvocationIndex + 2) - 1;
            tmp[j] += tmp[i];
        }
        offset <<= 1;
    }

    BARRIER();

	if (gl_LocalInvocationIndex == 0)
	{
		blockSum[gl_WorkGroupID.x] = tmp[BLOCKSIZE - 1];
		tmp[BLOCKSIZE - 1] = 0;
	}

    for (uint d = 1; d < BLOCKSIZE; d <<= 1)
    {
        offset >>= 1;
        BARRIER();

        if (gl_LocalInvocationIndex < d)
        {
            uint i = offset * (2 * gl_LocalInvocationIndex + 1) - 1;
            uint j = offset * (2 * gl_LocalInvocationIndex + 2) - 1;
            uint t = tmp[i];
            tmp[i] = tmp[j];
            tmp[j] += t;
        }
    }

    BARRIER();

    prefixSum[gl_WorkGroupID.x * BLOCKSIZE + gl_LocalInvocationIndex] = tmp[gl_LocalInvocationIndex];
    prefixSum[gl_WorkGroupID.x * BLOCKSIZE + gl_LocalInvocationIndex + HALFBLOCKSIZE] = tmp[gl_LocalInvocationIndex + HALFBLOCKSIZE];
}
						)"),
						PBF_DESC_DEBUG_NAME("Scan Compute Shader")
					}),
				.entryPoint = "main",
				.specialization = {
					Specialization<uint32_t>{.constantID = 0, .value = blockSize / 2 }
				}
			},
			.pipelineLayout = sortPipelineLayout,
			PBF_DESC_DEBUG_NAME("scan pipeline")
		}
	);
/*
	auto addBlockSumPipeline = cache().fetch(
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

layout (local_size_x_id = 0, local_size_y = 1, local_size_z = 1) in;

const uint HALFBLOCKSIZE = gl_WorkGroupSize.x;
const uint BLOCKSIZE = 2 * HALFBLOCKSIZE;


layout(std430, binding = 1) buffer PrefixSums
{
    uint prefixSum[];
};

layout(std430, binding = 2) readonly buffer BlockSums
{
    uint blockSum[];
};

void main()
{
	prefixSum[gl_GlobalInvocationID.x] += blockSum[gl_WorkGroupID.x];
}
						)"),
						PBF_DESC_DEBUG_NAME("Add Block Sum Compute Shader")
					}),
				.entryPoint = "main",
				.specialization = {
					Specialization<uint32_t>{.constantID = 0, .value = blockSize }
				}
			},
			.pipelineLayout = sortPipelineLayout,
			PBF_DESC_DEBUG_NAME("add block sum pipeline")
		}
	);
*/

	auto globalSortPipeline = cache().fetch(
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

layout (local_size_x_id = 0, local_size_y = 1, local_size_z = 1) in;

const uint BLOCKSIZE = gl_WorkGroupSize.x;

layout(std430, binding = 0) readonly buffer Keys
{
    uint keys[];
};

layout(std430, binding = 1) readonly buffer PrefixSums
{
    uint prefixSum[];
};

layout(std430, binding = 2) readonly buffer BlockSums
{
    uint blockSum[];
};

layout(std430, binding = 3) writeonly buffer Result
{
    uint result[];
};

uint GetHash(uint id) {
	return keys[id];
}

layout(push_constant) uniform constants {
	uvec4 blockSumOffsets;
    int bitShift;
};

void main()
{
	uint bits = bitfieldExtract(GetHash(gl_GlobalInvocationID.x), bitShift, 2);

	result[blockSum[blockSumOffsets[bits] + gl_WorkGroupID.x] + prefixSum[gl_GlobalInvocationID.x]] = keys[gl_GlobalInvocationID.x];
	//result[gl_GlobalInvocationID.x] = blockSum[blockSumOffsets[bits] + gl_WorkGroupID.x] + prefixSum[gl_GlobalInvocationID.x];
}
						)"),
						PBF_DESC_DEBUG_NAME("Global Sort Compute Shader")
					}),
				.entryPoint = "main",
				.specialization = {
					Specialization<uint32_t>{.constantID = 0, .value = blockSize }
				}
			},
			.pipelineLayout = sortPipelineLayout,
			PBF_DESC_DEBUG_NAME("global sort pipeline")
		}
	);
	vk::UniqueDescriptorPool descriptorPool;
	{
		std::uint32_t numDescriptorSets = 1024;
		static std::array<vk::DescriptorPoolSize, 1> sizes {{{ vk::DescriptorType::eStorageBuffer, 1024 }}};
		descriptorPool = device().createDescriptorPoolUnique(vk::DescriptorPoolCreateInfo{
			.maxSets = numDescriptorSets,
			.poolSizeCount = static_cast<std::uint32_t>(sizes.size()),
			.pPoolSizes = sizes.data()
		});
	}

	auto prescanParams = device().allocateDescriptorSets(vk::DescriptorSetAllocateInfo{
		.descriptorPool = *descriptorPool,
		.descriptorSetCount = 1,
		.pSetLayouts = &*sortLayout
	}).front();
	{
		auto descriptorInfos = {
			vk::DescriptorBufferInfo{keys.buffer(), 0, keys.size()},
			vk::DescriptorBufferInfo{prefixSums.buffer(), 0, prefixSums.size()},
			vk::DescriptorBufferInfo{blockSums.buffer(), 0, blockSums.size()}
		};
		device().updateDescriptorSets({vk::WriteDescriptorSet{
			.dstSet = prescanParams,
			.dstBinding = 0,
			.dstArrayElement = 0,
			.descriptorCount = static_cast<uint32_t>(descriptorInfos.size()),
			.descriptorType = vk::DescriptorType::eStorageBuffer,
			.pImageInfo = nullptr,
			.pBufferInfo = &*descriptorInfos.begin(),
			.pTexelBufferView = nullptr
		}}, {});
	}

	auto scanParams = device().allocateDescriptorSets(vk::DescriptorSetAllocateInfo{
		.descriptorPool = *descriptorPool,
		.descriptorSetCount = 1,
		.pSetLayouts = &*sortLayout
	}).front();
	{
		auto descriptorInfos = {
			vk::DescriptorBufferInfo{blockSums.buffer(), 0, blockSums.size()},
			vk::DescriptorBufferInfo{blockSums2.buffer(), 0, blockSums2.size()}
		};
		device().updateDescriptorSets({vk::WriteDescriptorSet{
			.dstSet = scanParams,
			.dstBinding = 1,
			.dstArrayElement = 0,
			.descriptorCount = static_cast<uint32_t>(descriptorInfos.size()),
			.descriptorType = vk::DescriptorType::eStorageBuffer,
			.pImageInfo = nullptr,
			.pBufferInfo = &*descriptorInfos.begin(),
			.pTexelBufferView = nullptr
		}}, {});
	}

	auto globalSortParams = device().allocateDescriptorSets(vk::DescriptorSetAllocateInfo{
		.descriptorPool = *descriptorPool,
		.descriptorSetCount = 1,
		.pSetLayouts = &*sortLayout
	}).front();
	{
		auto descriptorInfos = {
			vk::DescriptorBufferInfo{keys.buffer(), 0, keys.size()},
			vk::DescriptorBufferInfo{prefixSums.buffer(), 0, prefixSums.size()},
			vk::DescriptorBufferInfo{blockSums.buffer(), 0, blockSums.size()},
			vk::DescriptorBufferInfo{result.buffer(), 0, result.size()},
		};
		device().updateDescriptorSets({vk::WriteDescriptorSet{
			.dstSet = globalSortParams,
			.dstBinding = 0,
			.dstArrayElement = 0,
			.descriptorCount = static_cast<uint32_t>(descriptorInfos.size()),
			.descriptorType = vk::DescriptorType::eStorageBuffer,
			.pImageInfo = nullptr,
			.pBufferInfo = &*descriptorInfos.begin(),
			.pTexelBufferView = nullptr
		}}, {});
	}

	uint32_t barrierQueueFamily = VK_QUEUE_FAMILY_IGNORED;

	run([&](vk::CommandBuffer buf) {

		buf.pipelineBarrier(vk::PipelineStageFlagBits::eHost, vk::PipelineStageFlagBits::eComputeShader, {}, {
				vk::MemoryBarrier{
					.srcAccessMask = vk::AccessFlagBits::eHostWrite,
					.dstAccessMask = vk::AccessFlagBits::eShaderRead
				}
			}, {
			vk::BufferMemoryBarrier{
				.srcAccessMask = vk::AccessFlagBits::eHostWrite,
				.dstAccessMask = vk::AccessFlagBits::eShaderRead,
				.srcQueueFamilyIndex = barrierQueueFamily,
				.dstQueueFamilyIndex = barrierQueueFamily,
				.buffer = keys.buffer(),
				.offset = 0,
				.size = keys.size()
			}
		}, {});

		for (uint32_t bit = 0; bit < 32; bit += 2) {

		struct PushConstants {
			glm::u32vec4 blockSumOffset;
			uint32_t bit = 0;
		};
		PushConstants pushConstants {
			.blockSumOffset = glm::u32vec4{
				0, numGroups, numGroups * 2, numGroups * 3
			},
			.bit = bit
		};

		buf.bindPipeline(vk::PipelineBindPoint::eCompute, *prescanPipeline);
		buf.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *(prescanPipeline.descriptor().pipelineLayout), 0, {prescanParams}, {});
		buf.pushConstants(*(prescanPipeline.descriptor().pipelineLayout), vk::ShaderStageFlagBits::eCompute, 0, sizeof(pushConstants), &pushConstants);
		// reads keys; writes prefix sums and block sums
		buf.dispatch(((numKeys + blockSize - 1) / blockSize) * 2, 1, 1);

		buf.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader, {}, {
				vk::MemoryBarrier{
					.srcAccessMask = vk::AccessFlagBits::eShaderWrite,
					.dstAccessMask = vk::AccessFlagBits::eShaderRead
				}
			}, {
			vk::BufferMemoryBarrier{
				.srcAccessMask = vk::AccessFlagBits::eShaderWrite,
				.dstAccessMask = vk::AccessFlagBits::eShaderRead,
				.srcQueueFamilyIndex = barrierQueueFamily,
				.dstQueueFamilyIndex = barrierQueueFamily,
				.buffer = prefixSums.buffer(),
				.offset = 0,
				.size = prefixSums.size()
			},
			vk::BufferMemoryBarrier{
				.srcAccessMask = vk::AccessFlagBits::eShaderWrite,
				.dstAccessMask = vk::AccessFlagBits::eShaderRead,
				.srcQueueFamilyIndex = barrierQueueFamily,
				.dstQueueFamilyIndex = barrierQueueFamily,
				.buffer = blockSums.buffer(),
				.offset = 0,
				.size = blockSums.size()
			}
		}, {});

		buf.bindPipeline(vk::PipelineBindPoint::eCompute, *scanPipeline);
		buf.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *(scanPipeline.descriptor().pipelineLayout), 0, {scanParams}, {});
		// reads and writes block sums; writes block sums2
		buf.dispatch(2, 1, 1); // TODO

		buf.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader, {}, {
				vk::MemoryBarrier{
					.srcAccessMask = vk::AccessFlagBits::eShaderWrite,
					.dstAccessMask = vk::AccessFlagBits::eShaderRead
				}
			}, {
			/*vk::BufferMemoryBarrier{
				.srcAccessMask = vk::AccessFlagBits::eShaderWrite,
				.dstAccessMask = vk::AccessFlagBits::eShaderRead,
				.srcQueueFamilyIndex = barrierQueueFamily,
				.dstQueueFamilyIndex = barrierQueueFamily,
				.buffer = prefixSums.buffer(),
				.offset = 0,
				.size = prefixSums.size()
			},*/
			vk::BufferMemoryBarrier{
				.srcAccessMask = vk::AccessFlagBits::eShaderWrite,
				.dstAccessMask = vk::AccessFlagBits::eShaderRead,
				.srcQueueFamilyIndex = barrierQueueFamily,
				.dstQueueFamilyIndex = barrierQueueFamily,
				.buffer = blockSums.buffer(),
				.offset = 0,
				.size = blockSums.size()
			},
			vk::BufferMemoryBarrier{
				.srcAccessMask = vk::AccessFlagBits::eShaderWrite,
				.dstAccessMask = vk::AccessFlagBits::eShaderRead,
				.srcQueueFamilyIndex = barrierQueueFamily,
				.dstQueueFamilyIndex = barrierQueueFamily,
				.buffer = blockSums2.buffer(),
				.offset = 0,
				.size = blockSums2.size()
			}
		}, {});

		buf.bindPipeline(vk::PipelineBindPoint::eCompute, *globalSortPipeline);
		buf.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *(globalSortPipeline.descriptor().pipelineLayout), 0, {globalSortParams}, {});
		buf.pushConstants(*(globalSortPipeline.descriptor().pipelineLayout), vk::ShaderStageFlagBits::eCompute, 0, sizeof(pushConstants), &pushConstants);
		// reads keys, prefix sums and block sums; writes result
		buf.dispatch(((numKeys + blockSize - 1) / blockSize), 1, 1);

		buf.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eTransfer, {}, {
				vk::MemoryBarrier{
					.srcAccessMask = vk::AccessFlagBits::eShaderWrite,
					.dstAccessMask = vk::AccessFlagBits::eTransferRead
				}
			}, {
			vk::BufferMemoryBarrier{
				.srcAccessMask = vk::AccessFlagBits::eShaderWrite,
				.dstAccessMask = vk::AccessFlagBits::eTransferRead,
				.srcQueueFamilyIndex = barrierQueueFamily,
				.dstQueueFamilyIndex = barrierQueueFamily,
				.buffer = result.buffer(),
				.offset = 0,
				.size = result.size()
			}
		}, {});

		// reads result; writes keys
		buf.copyBuffer(result.buffer(), keys.buffer(), {
			vk::BufferCopy{
				.srcOffset = 0,
				.dstOffset = 0,
				.size = result.size()
			}
		});


		buf.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eComputeShader, {}, {
			vk::MemoryBarrier{
				.srcAccessMask = vk::AccessFlagBits::eTransferWrite,
				.dstAccessMask = vk::AccessFlagBits::eShaderRead
			}
		}, {
			vk::BufferMemoryBarrier{
				.srcAccessMask = vk::AccessFlagBits::eTransferWrite,
				.dstAccessMask = vk::AccessFlagBits::eShaderRead,
				.srcQueueFamilyIndex = barrierQueueFamily,
				.dstQueueFamilyIndex = barrierQueueFamily,
				.buffer = keys.buffer(),
				.offset = 0,
				.size = keys.size()
			}
		}, {});

		}

		buf.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eHost, {}, {
				vk::MemoryBarrier{
					.srcAccessMask = vk::AccessFlagBits::eTransferWrite,
					.dstAccessMask = vk::AccessFlagBits::eHostRead
				}
			}, {
			vk::BufferMemoryBarrier{
				.srcAccessMask = vk::AccessFlagBits::eTransferWrite,
				.dstAccessMask = vk::AccessFlagBits::eHostRead,
				.srcQueueFamilyIndex = barrierQueueFamily,
				.dstQueueFamilyIndex = barrierQueueFamily,
				.buffer = keys.buffer(),
				.offset = 0,
				.size = keys.size()
			}
		}, {});

	});

	device().waitIdle();
	keys.flush();
	device().waitIdle();
/*
	{
		UNSCOPED_INFO("prefixSums");
		uint32_t *data = prefixSums.as<uint32_t>();
		for (size_t i = 0; i < prefixSums.size() / sizeof(uint32_t); ++i)
			UNSCOPED_INFO(std::to_string(i) + ": " + std::to_string(data[i]));
		CHECK(false);
	}
	{
		UNSCOPED_INFO("blockSums");
		uint32_t *data = blockSums.as<uint32_t>();
		for (size_t i = 0; i < blockSums.size() / sizeof(uint32_t); ++i)
			UNSCOPED_INFO(std::to_string(i) + ": " + std::to_string(data[i]));
		CHECK(false);
	}
	{
		UNSCOPED_INFO("blockSums2");
		uint32_t *data = blockSums2.as<uint32_t>();
		for (size_t i = 0; i < blockSums2.size() / sizeof(uint32_t); ++i)
			UNSCOPED_INFO(std::to_string(i) + ": " + std::to_string(data[i]));
		CHECK(false);
	}
	{
		UNSCOPED_INFO("result");
		uint32_t *data = result.as<uint32_t>();
		for (size_t i = 0; i < result.size() / sizeof(uint32_t); ++i)
			UNSCOPED_INFO(std::to_string(i) + ": " + std::to_string(data[i]));
		CHECK(false);
	}*/
	{
		UNSCOPED_INFO("keys");
		uint32_t *data = keys.as<uint32_t>();
		size_t numKeys = keys.size() / sizeof(uint32_t);
		std::sort(initialKeys.begin(), initialKeys.end());
		for (size_t i = 0; i < numKeys; ++i)
			UNSCOPED_INFO(std::to_string(i) + ": " + std::to_string(data[i]) +
							  (i < numKeys - 1 ? (data[i] <= data[i+1] ? " <=" : " >") : "") +
							  "    [" + (data[i] == initialKeys[i] ? "==" : "!=") + " reference (" + std::to_string(initialKeys[i]) + ")]"
			);
		CHECK(false);
	}
}
