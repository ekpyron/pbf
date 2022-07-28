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
#include <random>

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
#version 460
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


uint32_t bitfieldExtract(uint32_t value, int32_t offset, int32_t size) {
	return (value >> offset) & ((1 << size) - 1);
}

TEST_CASE_METHOD(ComputeShaderUnitTest, "Bitfield Extract", "[bitfield extract]")
{
	CHECK(bitfieldExtract(0, 0, 2) == 0);
	CHECK(bitfieldExtract(1, 0, 2) == 1);
	CHECK(bitfieldExtract(2, 0, 2) == 2);
	CHECK(bitfieldExtract(3, 0, 2) == 3);

	CHECK(bitfieldExtract(4, 0, 2) == 0);
	CHECK(bitfieldExtract(5, 0, 2) == 1);
	CHECK(bitfieldExtract(6, 0, 2) == 2);
	CHECK(bitfieldExtract(7, 0, 2) == 3);

	CHECK(bitfieldExtract(4, 2, 2) == 1);
	CHECK(bitfieldExtract(5, 2, 2) == 1);
	CHECK(bitfieldExtract(6, 2, 2) == 1);
	CHECK(bitfieldExtract(7, 2, 2) == 1);

	CHECK(bitfieldExtract(0b0000, 2, 2) == 0b00);
	CHECK(bitfieldExtract(0b0100, 2, 2) == 0b01);
	CHECK(bitfieldExtract(0b1000, 2, 2) == 0b10);
	CHECK(bitfieldExtract(0b1100, 2, 2) == 0b11);
}


TEST_CASE("CPU Sort Test", "[cpu sort]")
{
	std::vector<uint32_t> initialKeys;
	{
		std::random_device rd;  //Will be used to obtain a seed for the random number engine
		std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
		std::uniform_int_distribution<uint32_t> distrib(0u, std::numeric_limits<uint32_t>::max());
		for (size_t i = 0; i < 32 * 32 * 33 + 16; ++i)
			initialKeys.emplace_back(distrib(gen));
	}
	const uint32_t numKeys = static_cast<uint32_t>(initialKeys.size());
	const uint32_t sortBitCount = 32;
	const uint32_t blockSize = 16;
	const uint32_t BLOCKSIZE = blockSize;
	const uint32_t halfBlockSize = blockSize / 2;
	const uint32_t HALFBLOCKSIZE = halfBlockSize;
	const uint32_t numGroups = (numKeys + blockSize - 1) / blockSize;

	const uint32_t LOG2HALFBLOCKSIZE = []()
	{
		uint result = 0;
		for (uint d = blockSize / 2; d > 0; d /= 2)
			result++;
		return result;
	}();
	const uint32_t LOG2BLOCKSIZE = []()
	{
		uint result = 0;
		for (uint d = 1; d < blockSize; d *= 2)
			result++;
		return result;
	}();

	auto keys = initialKeys;
	auto GetHash = [&keys](uint32_t index) {
		return keys[index];
	};

	std::vector<uint32_t> result;
	result.resize(keys.size());


	std::vector<uint32_t> prefixSum;
	prefixSum.resize(numKeys);

	std::vector<std::vector<uint32_t>> blockSums;
	for (ssize_t blockSumSize = 4 * numGroups; blockSumSize > 1; blockSumSize = (blockSumSize + blockSize - 1) / blockSize) {
		blockSums.emplace_back((blockSumSize + blockSize - 1)&~(blockSize - 1));
	}
	blockSums.emplace_back(blockSize);

	auto& prescanBlockSum = blockSums.front();

	const glm::u32vec4 blockSumOffsets = {
		0, numGroups, numGroups * 2, numGroups * 3
	};

	for (uint32_t bit = 0; bit < sortBitCount; bit += 2)
	{
		uint32_t bitShift = bit;
		using uvec4 = glm::uvec4;

		// prescan
		for (uint32_t workGroupID = 0; workGroupID < numGroups; workGroupID++)
		{
			glm::uvec3 gl_WorkGroupID(workGroupID, 0, 0);

			glm::uvec4 tmp[blockSize];
			for (uint32_t gl_LocalInvocationIndex = 0; gl_LocalInvocationIndex < halfBlockSize; gl_LocalInvocationIndex++)
			{
				uint bits1 =
					bitfieldExtract(GetHash(gl_WorkGroupID.x * BLOCKSIZE + gl_LocalInvocationIndex), bitShift, 2);
				uint bits2 = bitfieldExtract(
					GetHash(gl_WorkGroupID.x * BLOCKSIZE + gl_LocalInvocationIndex + HALFBLOCKSIZE),
					bitShift,
					2
				);

				tmp[gl_LocalInvocationIndex] = uvec4(equal(bits1 * uvec4(1, 1, 1, 1), uvec4(0, 1, 2, 3)));
				tmp[gl_LocalInvocationIndex + HALFBLOCKSIZE] =
					uvec4(equal(bits2 * uvec4(1, 1, 1, 1), uvec4(0, 1, 2, 3)));
			}
			int offset = 1;
			uint d = HALFBLOCKSIZE;
			for (uint c = 0; c < LOG2HALFBLOCKSIZE; c++)
			{
				for (uint32_t gl_LocalInvocationIndex = 0; gl_LocalInvocationIndex < halfBlockSize; gl_LocalInvocationIndex++)
				{
					{
						if (gl_LocalInvocationIndex < d)
						{
							uint i = offset * (2 * gl_LocalInvocationIndex + 1) - 1;
							uint j = offset * (2 * gl_LocalInvocationIndex + 2) - 1;
							tmp[j] += tmp[i];
						}
					}
				}
				offset *= 2;
				d /= 2;
			}
			for (uint32_t gl_LocalInvocationIndex = 0; gl_LocalInvocationIndex < halfBlockSize; gl_LocalInvocationIndex++)
			{
				if (gl_LocalInvocationIndex == 0)
				{
					for (uint i = 0; i < 4; ++i)
					{
						auto ix = uvec4(blockSumOffsets)[i] + gl_WorkGroupID.x;
						prescanBlockSum[ix] = uvec4(tmp[BLOCKSIZE - 1])[i];
					}
					tmp[BLOCKSIZE - 1] = uvec4(0, 0, 0, 0);
				}
			}
			d = 1;
			for (uint ctr = 0; ctr < LOG2BLOCKSIZE; ctr++)
			{
				offset /= 2;

				for (uint32_t gl_LocalInvocationIndex = 0; gl_LocalInvocationIndex < halfBlockSize; gl_LocalInvocationIndex++)
				{
					if (gl_LocalInvocationIndex < d)
					{
						uint i = offset * (2 * gl_LocalInvocationIndex + 1) - 1;
						uint j = offset * (2 * gl_LocalInvocationIndex + 2) - 1;
						uvec4 t = tmp[i];
						tmp[i] = tmp[j];
						tmp[j] += t;
					}
				}
				d *= 2;
			}

			for (uint32_t gl_LocalInvocationIndex = 0; gl_LocalInvocationIndex < halfBlockSize; gl_LocalInvocationIndex++)
			{
				uint bits1 =
					bitfieldExtract(GetHash(gl_WorkGroupID.x * BLOCKSIZE + gl_LocalInvocationIndex), bitShift, 2);
				uint bits2 = bitfieldExtract(
					GetHash(gl_WorkGroupID.x * BLOCKSIZE + gl_LocalInvocationIndex + HALFBLOCKSIZE),
					bitShift,
					2
				);


				prefixSum[gl_WorkGroupID.x * BLOCKSIZE + gl_LocalInvocationIndex] = uvec4(tmp[gl_LocalInvocationIndex])[bits1];
				prefixSum[gl_WorkGroupID.x * BLOCKSIZE + gl_LocalInvocationIndex + HALFBLOCKSIZE] = uvec4(tmp[gl_LocalInvocationIndex + HALFBLOCKSIZE])[bits2];

			}
		}

		// scan
		auto scan = [BLOCKSIZE, HALFBLOCKSIZE, LOG2BLOCKSIZE, LOG2HALFBLOCKSIZE](auto& prefixSum, auto& blockSum) {
			// const uint32_t numBlockSums_alt = (4 * numGroups + blockSize - 1) / blockSize;
			const uint32_t numBlockSums = (prefixSum.size() + blockSize - 1) / blockSize;
			for (uint32_t workGroupID = 0; workGroupID < numBlockSums; workGroupID++)
			{

				glm::uvec3 gl_WorkGroupID(workGroupID, 0, 0);

				uint32_t tmp[blockSize];
				for (uint32_t gl_LocalInvocationIndex = 0; gl_LocalInvocationIndex < halfBlockSize; gl_LocalInvocationIndex++)
				{
					tmp[gl_LocalInvocationIndex] = prefixSum[gl_WorkGroupID.x * BLOCKSIZE + gl_LocalInvocationIndex];
					tmp[gl_LocalInvocationIndex + HALFBLOCKSIZE] =
						prefixSum[gl_WorkGroupID.x * BLOCKSIZE + gl_LocalInvocationIndex + HALFBLOCKSIZE];
				}
				int offset = 1;
				uint d = HALFBLOCKSIZE;
				for (uint ctr = 0; ctr < LOG2HALFBLOCKSIZE; ctr++)
				{
					for (uint32_t gl_LocalInvocationIndex = 0; gl_LocalInvocationIndex < halfBlockSize; gl_LocalInvocationIndex++)
					{
						{
							{
								if (gl_LocalInvocationIndex < d)
								{
									uint i = offset * (2 * gl_LocalInvocationIndex + 1) - 1;
									uint j = offset * (2 * gl_LocalInvocationIndex + 2) - 1;
									tmp[j] += tmp[i];
								}
							}
						}
					}
					offset *= 2;
					d /= 2;
				}
				for (uint32_t gl_LocalInvocationIndex = 0; gl_LocalInvocationIndex < halfBlockSize; gl_LocalInvocationIndex++)
				{
					if (gl_LocalInvocationIndex == 0)
					{
						blockSum[gl_WorkGroupID.x] = tmp[BLOCKSIZE - 1];
						tmp[BLOCKSIZE - 1] = 0;
					}
				}

				d = 1;
				{
					{
						for (uint ctr = 0; ctr < LOG2BLOCKSIZE; ctr++)
						{
							offset /= 2;
							for (uint32_t gl_LocalInvocationIndex = 0; gl_LocalInvocationIndex < halfBlockSize; gl_LocalInvocationIndex++)
							{
								if (gl_LocalInvocationIndex < d)
								{
									uint i = offset * (2 * gl_LocalInvocationIndex + 1) - 1;
									uint j = offset * (2 * gl_LocalInvocationIndex + 2) - 1;
									uint t = tmp[i];
									tmp[i] = tmp[j];
									tmp[j] += t;
								}
							}
							d *= 2;
						}
					}
				}

				for (uint32_t gl_LocalInvocationIndex = 0; gl_LocalInvocationIndex < halfBlockSize; gl_LocalInvocationIndex++)
				{
					auto ix = gl_WorkGroupID.x * BLOCKSIZE + gl_LocalInvocationIndex;
					prefixSum[ix] = tmp[gl_LocalInvocationIndex];
					ix = gl_WorkGroupID.x * BLOCKSIZE + gl_LocalInvocationIndex + HALFBLOCKSIZE;
					prefixSum.at(ix) = tmp[gl_LocalInvocationIndex + HALFBLOCKSIZE];
				}
			}
		};

		for (size_t i = 0; i < blockSums.size() - 1; ++i) {
			scan(blockSums[i], blockSums[i + 1]);
		}

		auto addblocksums = [](auto& prefixSum, auto const& blockSum) {
			const uint32_t numBlockSums = (prefixSum.size() + blockSize - 1) / blockSize;
			for (uint32_t workGroupID = 0; workGroupID < numBlockSums; workGroupID++)
			{
				for (uint32_t gl_LocalInvocationIndex = 0; gl_LocalInvocationIndex < blockSize; gl_LocalInvocationIndex++)
				{
					prefixSum[workGroupID * blockSize + gl_LocalInvocationIndex] += blockSum[workGroupID];
				}
			}
		};

		for (ssize_t i = blockSums.size() - 2; i > 0; i--) {
			addblocksums(blockSums[i - 1], blockSums[i]);
		}

/*		std::vector<uint32_t> blockSum2((blockSum.size() + blockSize - 1) / blockSize);

		scan(blockSum, blockSum2);*/

		// TODO: propagate block sums back down, i.e. addblocksum

		// globalsort
		for (uint32_t workGroupID = 0; workGroupID < numGroups; workGroupID++)
		{
			for (uint32_t gl_LocalInvocationIndex = 0; gl_LocalInvocationIndex < blockSize; gl_LocalInvocationIndex++)
			{
				uint32_t globalInvocationID = workGroupID * blockSize + gl_LocalInvocationIndex;

				uint bits = bitfieldExtract(GetHash(globalInvocationID), bitShift, 2);

				result[prescanBlockSum[blockSumOffsets[bits] + workGroupID] + prefixSum[globalInvocationID]] = keys[globalInvocationID];

			}
		}

		keys = result;
	}

	std::sort(std::begin(initialKeys), std::end(initialKeys));
	for (size_t i = 0; i < keys.size(); ++i) {
		CHECK(keys[i] == initialKeys[i]);
	}
}

TEST_CASE_METHOD(ComputeShaderUnitTest, "Compute Shader Sort Test", "[sort]")
{
	using namespace pbf;


	std::vector<uint32_t> initialKeys;

	{
		std::random_device rd;  //Will be used to obtain a seed for the random number engine
		std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
		std::uniform_int_distribution<uint32_t> distrib(0, std::numeric_limits<uint32_t>::max());
		for (size_t i = 0; i < 32 * 32 * 32 + 32 + 32 + 76; ++i)
			initialKeys.emplace_back(distrib(gen));

	}

	/*{
		auto rd = std::random_device {};
		auto rng = std::default_random_engine { rd() };
		std::shuffle(std::begin(initialKeys), std::end(initialKeys), rng);
	}*/
	/*for (auto v: initialKeys)
		UNSCOPED_INFO("Key: " + std::to_string(v));*/

	const uint32_t blockSize = 32; // apparently has to be >= initialKeys.size() / 2 right now

	while (initialKeys.size() % blockSize) {
		initialKeys.emplace_back(std::numeric_limits<uint32_t>::max());
	}
	
	const uint32_t numKeys = static_cast<uint32_t>(initialKeys.size());


	const uint32_t sortBitCount = 32; // sort up to # of bits
	struct PushConstants {
		glm::u32vec4 blockSumOffset;
		uint32_t bit = 0;
	};

	device().waitIdle();
	Buffer hostBuffer{
		this,
		numKeys * sizeof(uint32_t),
		vk::BufferUsageFlagBits::eTransferSrc|vk::BufferUsageFlagBits::eTransferDst,
		MemoryType::DYNAMIC
	};
	Buffer keys{
		this,
		numKeys * sizeof(uint32_t),
		vk::BufferUsageFlagBits::eStorageBuffer|vk::BufferUsageFlagBits::eTransferDst|vk::BufferUsageFlagBits::eTransferSrc,
		MemoryType::STATIC
	};
	device().waitIdle();
	{
		uint32_t *data = hostBuffer.as<uint32_t>();
		for (uint32_t i = 0; i < numKeys; ++i)
			data[i] = initialKeys[i];
		hostBuffer.flush();
	}
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

	const uint32_t numGroups = (numKeys + blockSize - 1) / blockSize;

	std::vector<Buffer> blockSums;
	for (ssize_t blockSumSize = 4 * numGroups; blockSumSize > 1; blockSumSize = (blockSumSize + blockSize - 1) / blockSize) {
		blockSums.emplace_back(
			this,
			((blockSumSize + blockSize - 1) & ~(blockSize - 1)) * sizeof(uint32_t),
			vk::BufferUsageFlagBits::eStorageBuffer,
			MemoryType::STATIC
		);
	}
	blockSums.emplace_back(this, blockSize * sizeof(uint32_t), vk::BufferUsageFlagBits::eStorageBuffer, MemoryType::STATIC);

	Buffer& prescanBlockSum = blockSums.front();

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
				sizeof(PushConstants)
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
#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_KHR_shader_subgroup_basic : enable

layout (local_size_x_id = 0, local_size_y = 1, local_size_z = 1) in;

precision highp float;
precision highp int;

const uint HALFBLOCKSIZE = gl_WorkGroupSize.x;
const uint BLOCKSIZE = 2 * HALFBLOCKSIZE;
layout (constant_id = 1) const uint LOG2HALFBLOCKSIZE = 0;
layout (constant_id = 2) const uint LOG2BLOCKSIZE = 0;

layout(std430, binding = 0) readonly volatile coherent  buffer Keys
{
    uint keys[];
};

layout(std430, binding = 1) writeonly volatile coherent  buffer PrefixSums
{
    uint prefixSum[];
};

layout(std430, binding = 2) writeonly volatile coherent  buffer BlockSums
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
//#define BARRIER() memoryBarrier(); groupMemoryBarrier(); barrier()
#define BARRIER() memoryBarrier(); barrier()
//#define BARRIER() barrier()

void main()
{
	uint bits1 = bitfieldExtract(GetHash(gl_WorkGroupID.x * BLOCKSIZE + gl_LocalInvocationIndex), bitShift, 2);
	uint bits2 = bitfieldExtract(GetHash(gl_WorkGroupID.x * BLOCKSIZE + gl_LocalInvocationIndex + HALFBLOCKSIZE), bitShift, 2);

	tmp[gl_LocalInvocationIndex] = uvec4(equal(bits1 * uvec4(1,1,1,1), uvec4(0,1,2,3)));
	tmp[gl_LocalInvocationIndex + HALFBLOCKSIZE] = uvec4(equal(bits2 * uvec4(1,1,1,1), uvec4(0,1,2,3)));


    int offset = 1;
	{
		uint d = HALFBLOCKSIZE;
		for (uint c = 0; c < LOG2HALFBLOCKSIZE; c++)
		{
			BARRIER();

			if (gl_LocalInvocationIndex < d) {
				uint i = offset * (2 * gl_LocalInvocationIndex + 1) - 1;
				uint j = offset * (2 * gl_LocalInvocationIndex + 2) - 1;
				tmp[j] += tmp[i];
			}
			offset *= 2;
			d /= 2;
		}
	}

	BARRIER();

	if (gl_LocalInvocationIndex == 0)
	{
		for(uint i = 0; i < 4; ++i)
			blockSum[uvec4(blockSumOffsets)[i] + gl_WorkGroupID.x] = uvec4(tmp[BLOCKSIZE - 1])[i];
		tmp[BLOCKSIZE - 1] = uvec4(0, 0, 0, 0);
	}

	{
		uint d = 1;
		for (uint ctr = 0; ctr < LOG2BLOCKSIZE; ctr++)
		{
			offset /= 2;
			BARRIER();

			if (gl_LocalInvocationIndex < d)
			{
				uint i = offset * (2 * gl_LocalInvocationIndex + 1) - 1;
				uint j = offset * (2 * gl_LocalInvocationIndex + 2) - 1;
				uvec4 t = tmp[i];
				tmp[i] = tmp[j];
				tmp[j] += t;
			}
			d *= 2;
		}
	}

    BARRIER();

    prefixSum[gl_WorkGroupID.x * BLOCKSIZE + gl_LocalInvocationIndex] = uvec4(tmp[gl_LocalInvocationIndex])[bits1];
    prefixSum[gl_WorkGroupID.x * BLOCKSIZE + gl_LocalInvocationIndex + HALFBLOCKSIZE] = uvec4(tmp[gl_LocalInvocationIndex + HALFBLOCKSIZE])[bits2];
}
						)"),
						PBF_DESC_DEBUG_NAME("Prescan Compute Shader")
					}),
				.entryPoint = "main",
				.specialization = {
					Specialization<uint32_t>{.constantID = 0, .value = blockSize / 2 },
					Specialization<uint32_t>{.constantID = 1, .value = [&]() {
						uint result = 0;
						for (uint d = blockSize / 2; d > 0; d /= 2)
							result++;
						return result;
					}()},
					Specialization<uint32_t>{.constantID = 2, .value = [&]() {
						uint result = 0;
						for (uint d = 1; d < blockSize; d *= 2)
							result++;
						return result;
					}()}
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
#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_KHR_shader_subgroup_basic : enable

layout (local_size_x_id = 0, local_size_y = 1, local_size_z = 1) in;

precision highp float;
precision highp int;

const uint HALFBLOCKSIZE = gl_WorkGroupSize.x;
const uint BLOCKSIZE = 2 * HALFBLOCKSIZE;
layout (constant_id = 1) const uint LOG2HALFBLOCKSIZE = 0;
layout (constant_id = 2) const uint LOG2BLOCKSIZE = 0;

// TODO: what *exactly* is needed :-)?
//#define BARRIER() barrier(); memoryBarrier(); memoryBarrierShared(); groupMemoryBarrier(); barrier()
//#define BARRIER() memoryBarrier(); groupMemoryBarrier(); barrier()
#define BARRIER() memoryBarrier(); barrier()
//#define BARRIER() barrier()

layout(std430, binding = 1) volatile coherent buffer PrefixSums
{
    uint prefixSum[];
};

layout(std430, binding = 2) volatile coherent buffer BlockSums
{
    uint blockSum[];
};

shared uint tmp[BLOCKSIZE];

void main()
{
	tmp[gl_LocalInvocationIndex] = prefixSum[gl_WorkGroupID.x * BLOCKSIZE + gl_LocalInvocationIndex];
	tmp[gl_LocalInvocationIndex + HALFBLOCKSIZE] = prefixSum[gl_WorkGroupID.x * BLOCKSIZE + gl_LocalInvocationIndex + HALFBLOCKSIZE];


    int offset = 1;
	{
		uint d = HALFBLOCKSIZE;
#pragma unroll_loop_start
		for (uint ctr = 0; ctr < LOG2HALFBLOCKSIZE; ctr++)
		{
			BARRIER();

			if (gl_LocalInvocationIndex < d) {
				uint i = offset * (2 * gl_LocalInvocationIndex + 1) - 1;
				uint j = offset * (2 * gl_LocalInvocationIndex + 2) - 1;
				tmp[j] += tmp[i];
			}
			offset *= 2;
			d /= 2;
		}
#pragma unroll_loop_end
	}

    BARRIER();

	if (gl_LocalInvocationIndex == 0)
	{
		blockSum[gl_WorkGroupID.x] = tmp[BLOCKSIZE - 1];
		tmp[BLOCKSIZE - 1] = 0;
	}
    BARRIER();

	{
		uint d = 1;
#pragma unroll_loop_start
		for (uint ctr = 0; ctr < LOG2BLOCKSIZE; ctr++)
		{
			offset /= 2;
			BARRIER();

			if (gl_LocalInvocationIndex < d)
			{
				uint i = offset * (2 * gl_LocalInvocationIndex + 1) - 1;
				uint j = offset * (2 * gl_LocalInvocationIndex + 2) - 1;
				uint t = tmp[i];
				tmp[i] = tmp[j];
				tmp[j] += t;
			}
			d *= 2;
		}
#pragma unroll_loop_end
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
					Specialization<uint32_t>{.constantID = 0, .value = blockSize / 2 },
					Specialization<uint32_t>{.constantID = 1, .value = [&]() {
						uint result = 0;
						for (uint d = blockSize / 2; d > 0; d /= 2)
							result++;
						return result;
					}()},
					Specialization<uint32_t>{.constantID = 2, .value = [&]() {
						uint result = 0;
						for (uint d = 1; d < blockSize; d *= 2)
							result++;
						return result;
					}()}
				}
			},
			.pipelineLayout = sortPipelineLayout,
			PBF_DESC_DEBUG_NAME("scan pipeline")
		}
	);

	auto addBlockSumPipeline = cache().fetch(
		descriptors::ComputePipeline{
			.flags = {},
			.shaderStage = descriptors::ShaderStage {
				.stage = vk::ShaderStageFlagBits::eCompute,
				.module = cache().fetch(
					descriptors::ShaderModule{
						.source = compileComputeShader(R"(
#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_KHR_shader_subgroup_basic : enable

layout (local_size_x_id = 0, local_size_y = 1, local_size_z = 1) in;

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


	auto globalSortPipeline = cache().fetch(
		descriptors::ComputePipeline{
			.flags = {},
			.shaderStage = descriptors::ShaderStage {
				.stage = vk::ShaderStageFlagBits::eCompute,
				.module = cache().fetch(
					descriptors::ShaderModule{
						.source = compileComputeShader(R"(
#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_KHR_shader_subgroup_basic : enable

precision highp float;
precision highp int;

layout (local_size_x_id = 0, local_size_y = 1, local_size_z = 1) in;

const uint BLOCKSIZE = gl_WorkGroupSize.x;

layout(std430, binding = 0) volatile coherent readonly buffer Keys
{
    uint keys[];
};

layout(std430, binding = 1) volatile coherent readonly buffer PrefixSums
{
    uint prefixSum[];
};

layout(std430, binding = 2) volatile coherent readonly buffer BlockSums
{
    uint blockSum[];
};

layout(std430, binding = 3) volatile coherent writeonly buffer Result
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

	//result[gl_GlobalInvocationID.x] = blockSum[blockSumOffsets[bits] + gl_WorkGroupID.x] + prefixSum[gl_GlobalInvocationID.x];

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
			vk::DescriptorBufferInfo{prescanBlockSum.buffer(), 0, prescanBlockSum.size()}
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

	std::vector sortLayouts(blockSums.size() - 1, *sortLayout);
	auto scanParams = device().allocateDescriptorSets(vk::DescriptorSetAllocateInfo{
		.descriptorPool = *descriptorPool,
		.descriptorSetCount = static_cast<uint32_t>(sortLayouts.size()),
		.pSetLayouts = sortLayouts.data()
	});
	for (size_t i = 0; i < blockSums.size() - 1; ++i)
	{
		auto descriptorInfos = {
			vk::DescriptorBufferInfo{blockSums[i].buffer(), 0, blockSums[i].size()},
			vk::DescriptorBufferInfo{blockSums[i + 1].buffer(), 0, blockSums[i + 1].size()}
		};
		device().updateDescriptorSets({vk::WriteDescriptorSet{
			.dstSet = scanParams[i],
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
			vk::DescriptorBufferInfo{prescanBlockSum.buffer(), 0, prescanBlockSum.size()},
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

		buf.pipelineBarrier(vk::PipelineStageFlagBits::eHost, vk::PipelineStageFlagBits::eTransfer, {}, {
			vk::MemoryBarrier{
				.srcAccessMask = vk::AccessFlagBits::eHostWrite|vk::AccessFlagBits::eMemoryWrite,
				.dstAccessMask = vk::AccessFlagBits::eTransferRead|vk::AccessFlagBits::eMemoryRead
			}
		}, {}, {});

		buf.copyBuffer(hostBuffer.buffer(), keys.buffer(), {
			vk::BufferCopy{
				.srcOffset = 0,
				.dstOffset = 0,
				.size = hostBuffer.size()
			}
		});

		buf.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eComputeShader, {}, {
			vk::MemoryBarrier{
				.srcAccessMask = vk::AccessFlagBits::eTransferWrite|vk::AccessFlagBits::eMemoryWrite,
				.dstAccessMask = vk::AccessFlagBits::eShaderRead|vk::AccessFlagBits::eMemoryRead
			}
		}, {}, {});

		for (uint32_t bit = 0; bit < sortBitCount; bit += 2) {

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
		buf.dispatch(((numKeys + blockSize - 1) / blockSize), 1, 1);

		buf.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader, {}, {
				vk::MemoryBarrier{
					.srcAccessMask = vk::AccessFlagBits::eShaderWrite|vk::AccessFlagBits::eShaderRead|vk::AccessFlagBits::eMemoryWrite|vk::AccessFlagBits::eMemoryRead,
					.dstAccessMask = vk::AccessFlagBits::eShaderWrite|vk::AccessFlagBits::eShaderRead|vk::AccessFlagBits::eMemoryWrite|vk::AccessFlagBits::eMemoryRead
				}
			}, {}, {});

		for (size_t i = 0; i < blockSums.size() - 1; ++i)
		{
			auto& prefixSum = blockSums[i];
			auto& blockSum = blockSums[i + 1];
			const uint32_t numBlockSums = ((prefixSum.size() / sizeof(uint32_t)) + blockSize - 1) / blockSize;

			buf.bindPipeline(vk::PipelineBindPoint::eCompute, *scanPipeline);
			buf.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *(scanPipeline.descriptor().pipelineLayout), 0, {scanParams[i]}, {});
			buf.dispatch(numBlockSums, 1, 1);

			buf.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader, {}, {
				vk::MemoryBarrier{
					.srcAccessMask = vk::AccessFlagBits::eShaderWrite|vk::AccessFlagBits::eShaderRead|vk::AccessFlagBits::eMemoryWrite|vk::AccessFlagBits::eMemoryRead,
					.dstAccessMask = vk::AccessFlagBits::eShaderWrite|vk::AccessFlagBits::eShaderRead|vk::AccessFlagBits::eMemoryWrite|vk::AccessFlagBits::eMemoryRead
				}
			}, {}, {});
		}

		// TODO: addblocksums
		for (ssize_t i = blockSums.size() - 2; i > 0; i--) {
			auto& prefixSum = blockSums[i - 1];
			const uint32_t numBlockSums = ((prefixSum.size() / sizeof(uint32_t)) + blockSize - 1) / blockSize;

			buf.bindPipeline(vk::PipelineBindPoint::eCompute, *addBlockSumPipeline);
			buf.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *(scanPipeline.descriptor().pipelineLayout), 0, {scanParams[i - 1]}, {});
			buf.dispatch(numBlockSums, 1, 1);

			buf.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader, {}, {
				vk::MemoryBarrier{
					.srcAccessMask = vk::AccessFlagBits::eShaderWrite|vk::AccessFlagBits::eShaderRead|vk::AccessFlagBits::eMemoryWrite|vk::AccessFlagBits::eMemoryRead,
					.dstAccessMask = vk::AccessFlagBits::eShaderWrite|vk::AccessFlagBits::eShaderRead|vk::AccessFlagBits::eMemoryWrite|vk::AccessFlagBits::eMemoryRead
				}
			}, {}, {});
		}

		buf.bindPipeline(vk::PipelineBindPoint::eCompute, *globalSortPipeline);
		buf.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *(globalSortPipeline.descriptor().pipelineLayout), 0, {globalSortParams}, {});
		buf.pushConstants(*(globalSortPipeline.descriptor().pipelineLayout), vk::ShaderStageFlagBits::eCompute, 0, sizeof(pushConstants), &pushConstants);
		// reads keys, prefix sums and block sums; writes result
		buf.dispatch(((numKeys + blockSize - 1) / blockSize), 1, 1);

		buf.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eTransfer, {}, {
				vk::MemoryBarrier{
					.srcAccessMask = vk::AccessFlagBits::eShaderWrite|vk::AccessFlagBits::eShaderRead|vk::AccessFlagBits::eMemoryWrite|vk::AccessFlagBits::eMemoryRead,
					.dstAccessMask = vk::AccessFlagBits::eTransferRead|vk::AccessFlagBits::eMemoryRead
				}
			}, {}, {});

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
				.srcAccessMask = vk::AccessFlagBits::eTransferWrite|vk::AccessFlagBits::eMemoryWrite,
				.dstAccessMask = vk::AccessFlagBits::eShaderWrite|vk::AccessFlagBits::eShaderRead|vk::AccessFlagBits::eMemoryWrite|vk::AccessFlagBits::eMemoryRead
			}
		}, {}, {});

		} // END OF BIT LOOP

		buf.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, {}, {
			vk::MemoryBarrier{
				.srcAccessMask = vk::AccessFlagBits::eTransferWrite|vk::AccessFlagBits::eMemoryWrite,
				.dstAccessMask = vk::AccessFlagBits::eTransferRead|vk::AccessFlagBits::eMemoryRead
			}
		}, {}, {});

		buf.copyBuffer(keys.buffer(), hostBuffer.buffer(), {
			vk::BufferCopy{
				.srcOffset = 0,
				.dstOffset = 0,
				.size = hostBuffer.size()
			}
		});

		buf.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eHost, {}, {
			vk::MemoryBarrier{
				.srcAccessMask = vk::AccessFlagBits::eTransferWrite|vk::AccessFlagBits::eMemoryWrite,
				.dstAccessMask = vk::AccessFlagBits::eHostRead|vk::AccessFlagBits::eMemoryRead
			}
		}, {}, {});
	});

	device().waitIdle();
	hostBuffer.flush();
	device().waitIdle();

	{
		UNSCOPED_INFO("keys");
		uint32_t *data = hostBuffer.as<uint32_t>();
		size_t numKeys = hostBuffer.size() / sizeof(uint32_t);
		std::sort(initialKeys.begin(), initialKeys.end());
		bool allFine = true;
		for (size_t i = 0; i < numKeys; ++i)
		{
			/*UNSCOPED_INFO(std::to_string(i) + ": " + std::to_string(data[i]) +
						  (i < numKeys - 1 ? (data[i] <= data[i+1] ? " <=" : " >") : "") +
						  "    [" + (data[i] == initialKeys[i] ? "==" : "!=") + " reference (" + std::to_string(initialKeys[i]) + ")]"
			);*/
			if (data[i] != initialKeys[i])
				allFine = false;
		}
		CHECK(allFine);
	}
}
