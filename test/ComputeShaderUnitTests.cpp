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
#include <catch2/matchers/catch_matchers_templated.hpp>
#include "ComputeShaderUnitTestContext.h"
#include <pbf/RadixSort.h>
#include <ranges>
#include <random>
#include <span>

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


	Buffer<uint32_t> buffer{
		*this,
		4 * 4,
		vk::BufferUsageFlagBits::eStorageBuffer,
		MemoryType::DYNAMIC
	};
	{
		auto descriptorInfos = {vk::DescriptorBufferInfo{buffer.buffer(), 0, buffer.deviceSize()}};
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

	const auto *data = buffer.data();
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

auto roundToNextMultiple(auto value, auto mod) {
	if (value % mod) {
		return value + (mod - (value % mod));
	} else {
		return value;
	}
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
	const auto numKeys = static_cast<uint32_t>(initialKeys.size());
	const uint32_t sortBitCount = 32;
	const uint32_t blockSize = 16;
	const uint32_t BLOCKSIZE = blockSize;
	const uint32_t halfBlockSize = blockSize / 2;
	const uint32_t HALFBLOCKSIZE = halfBlockSize;
	const uint32_t numGroups = (numKeys + blockSize - 1) / blockSize;

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
		blockSums.emplace_back(roundToNextMultiple(blockSumSize, blockSize));
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
			for (uint d = blockSize / 2; d > 0; d /= 2)
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
			for (uint d = 1; d < blockSize; d *= 2)
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
		auto scan = [BLOCKSIZE, HALFBLOCKSIZE](auto& prefixSum, auto& blockSum) {
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
				for (uint d = blockSize / 2; d > 0; d /= 2)
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
				}
				for (uint32_t gl_LocalInvocationIndex = 0; gl_LocalInvocationIndex < halfBlockSize; gl_LocalInvocationIndex++)
				{
					if (gl_LocalInvocationIndex == 0)
					{
						blockSum[gl_WorkGroupID.x] = tmp[BLOCKSIZE - 1];
						tmp[BLOCKSIZE - 1] = 0;
					}
				}

				for (uint d = 1; d < blockSize; d *= 2)
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

template<typename Range>
struct EqualsRangeMatcher : Catch::Matchers::MatcherGenericBase {
	EqualsRangeMatcher(Range const& range):
		range{ range }
	{}

	template<typename OtherRange>
	bool match(OtherRange const& other) const {
		using std::begin; using std::end;

		return std::equal(begin(range), end(range), begin(other), end(other));
	}

	std::string describe() const override {
		return "Equals: " + Catch::rangeToString(range);
	}

private:
	Range const& range;
};

template<typename Range>
auto EqualsRange(const Range& range) -> EqualsRangeMatcher<Range> {
	return EqualsRangeMatcher<Range>{range};
}

TEST_CASE_METHOD(ComputeShaderUnitTest, "Compute Shader Sort Test Old Reference", "[sort_old_ref]")
{
	using namespace pbf;

	const size_t maxParticleCount = 128*128*128;
	const uint32_t sortBitCount = 32; // sort up to # of bits
	const uint32_t blockSize = 256;

	std::vector<uint32_t> initialKeys;

	{
		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_int_distribution<uint32_t> distrib(0, std::numeric_limits<uint32_t>::max());
		for (size_t i = 0; i < std::uniform_int_distribution<size_t>(0, maxParticleCount)(gen); ++i)
			initialKeys.emplace_back(distrib(gen));
	}


	if (initialKeys.size() % blockSize)
	{
		initialKeys.reserve(roundToNextMultiple(initialKeys.size(), blockSize));
		while (initialKeys.size() % blockSize)
			initialKeys.emplace_back(std::numeric_limits<uint32_t>::max());
	}

	const auto numKeys = static_cast<uint32_t>(initialKeys.size());


	struct PushConstants {
		glm::u32vec4 blockSumOffset;
		uint32_t bit = 0;
	};

	device().waitIdle();
	Buffer<uint32_t> hostBuffer{
		*this,
		numKeys,
		vk::BufferUsageFlagBits::eTransferSrc|vk::BufferUsageFlagBits::eTransferDst,
		MemoryType::DYNAMIC
	};
	Buffer<uint32_t> keys{
		*this,
		numKeys,
		vk::BufferUsageFlagBits::eStorageBuffer|vk::BufferUsageFlagBits::eTransferDst|vk::BufferUsageFlagBits::eTransferSrc,
		MemoryType::STATIC
	};
	device().waitIdle();
	{
		auto *data = hostBuffer.data();
		for (uint32_t i = 0; i < numKeys; ++i)
			data[i] = initialKeys[i];
		hostBuffer.flush();
	}
	device().waitIdle();
	Buffer<uint32_t> result{
		*this,
		numKeys,
		vk::BufferUsageFlagBits::eStorageBuffer|vk::BufferUsageFlagBits::eTransferSrc,
		MemoryType::STATIC
	};

	Buffer<uint32_t> prefixSums{
		*this,
		numKeys,
		vk::BufferUsageFlagBits::eStorageBuffer,
		MemoryType::STATIC
	};

	const uint32_t numGroups = (numKeys + blockSize - 1) / blockSize;

	std::vector<Buffer<uint32_t>> blockSums;
	for (ssize_t blockSumSize = 4 * numGroups; blockSumSize > 1; blockSumSize = (blockSumSize + blockSize - 1) / blockSize) {
		blockSums.emplace_back(
			*this,
			roundToNextMultiple(blockSumSize, blockSize),
			vk::BufferUsageFlagBits::eStorageBuffer,
			MemoryType::STATIC
		);
	}
	blockSums.emplace_back(*this, blockSize, vk::BufferUsageFlagBits::eStorageBuffer, MemoryType::STATIC);

	auto& prescanBlockSum = blockSums.front();

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

const uint HALFBLOCKSIZE = gl_WorkGroupSize.x;
const uint BLOCKSIZE = 2 * HALFBLOCKSIZE;

layout(std430, binding = 0) readonly buffer Keys
{
    uint keys[];
};

uint GetHash(uint id) {
	return keys[id];
}

layout(std430, binding = 1) writeonly buffer PrefixSums
{
    uint prefixSum[];
};

layout(std430, binding = 2) writeonly buffer BlockSums
{
    uint blockSum[];
};

layout(push_constant) uniform constants {
	uvec4 blockSumOffsets;
    int bitShift;
};

shared uvec4 tmp[BLOCKSIZE];

void main()
{
	uint bits1 = bitfieldExtract(GetHash(gl_WorkGroupID.x * BLOCKSIZE + gl_LocalInvocationIndex), bitShift, 2);
	uint bits2 = bitfieldExtract(GetHash(gl_WorkGroupID.x * BLOCKSIZE + gl_LocalInvocationIndex + HALFBLOCKSIZE), bitShift, 2);

	tmp[gl_LocalInvocationIndex] = uvec4(equal(bits1 * uvec4(1,1,1,1), uvec4(0,1,2,3)));
	tmp[gl_LocalInvocationIndex + HALFBLOCKSIZE] = uvec4(equal(bits2 * uvec4(1,1,1,1), uvec4(0,1,2,3)));


    int offset = 1;
	for (uint d = HALFBLOCKSIZE; d > 0; d /= 2)
	{
		barrier();

		if (gl_LocalInvocationIndex < d) {
			uint i = offset * (2 * gl_LocalInvocationIndex + 1) - 1;
			uint j = offset * (2 * gl_LocalInvocationIndex + 2) - 1;
			tmp[j] += tmp[i];
		}
		offset *= 2;
	}

	barrier();

	if (gl_LocalInvocationIndex == 0)
	{
		for(uint i = 0; i < 4; ++i)
			blockSum[blockSumOffsets[i] + gl_WorkGroupID.x] = tmp[BLOCKSIZE - 1][i];
		tmp[BLOCKSIZE - 1] = uvec4(0, 0, 0, 0);
	}

	for (uint d = 1; d < BLOCKSIZE; d *= 2)
	{
		offset /= 2;
		barrier();

		if (gl_LocalInvocationIndex < d)
		{
			uint i = offset * (2 * gl_LocalInvocationIndex + 1) - 1;
			uint j = offset * (2 * gl_LocalInvocationIndex + 2) - 1;
			uvec4 t = tmp[i];
			tmp[i] = tmp[j];
			tmp[j] += t;
		}
	}

    barrier();

    prefixSum[gl_WorkGroupID.x * BLOCKSIZE + gl_LocalInvocationIndex] = tmp[gl_LocalInvocationIndex][bits1];
    prefixSum[gl_WorkGroupID.x * BLOCKSIZE + gl_LocalInvocationIndex + HALFBLOCKSIZE] = tmp[gl_LocalInvocationIndex + HALFBLOCKSIZE][bits2];
}
						)"),
						PBF_DESC_DEBUG_NAME("Prescan Compute Shader")
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
#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_KHR_shader_subgroup_basic : enable

layout (local_size_x_id = 0, local_size_y = 1, local_size_z = 1) in;

const uint HALFBLOCKSIZE = gl_WorkGroupSize.x;
const uint BLOCKSIZE = 2 * HALFBLOCKSIZE;

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
	for (uint d = HALFBLOCKSIZE; d > 0; d /= 2)
	{
		barrier();

		if (gl_LocalInvocationIndex < d) {
			uint i = offset * (2 * gl_LocalInvocationIndex + 1) - 1;
			uint j = offset * (2 * gl_LocalInvocationIndex + 2) - 1;
			tmp[j] += tmp[i];
		}
		offset *= 2;
	}

    barrier();

	if (gl_LocalInvocationIndex == 0)
	{
		blockSum[gl_WorkGroupID.x] = tmp[BLOCKSIZE - 1];
		tmp[BLOCKSIZE - 1] = 0;
	}
    barrier();

	{
		for(uint d = 1; d < BLOCKSIZE; d *= 2)
		{
			offset /= 2;
			barrier();

			if (gl_LocalInvocationIndex < d)
			{
				uint i = offset * (2 * gl_LocalInvocationIndex + 1) - 1;
				uint j = offset * (2 * gl_LocalInvocationIndex + 2) - 1;
				uint t = tmp[i];
				tmp[i] = tmp[j];
				tmp[j] += t;
			}
		}
	}

    barrier();

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

uint GetPermutation(uint id) {
	uint bits = bitfieldExtract(GetHash(id), bitShift, 2);
	return blockSum[blockSumOffsets[bits] + gl_WorkGroupID.x] + prefixSum[gl_GlobalInvocationID.x];
}

void main()
{
	result[GetPermutation(gl_GlobalInvocationID.x)] = keys[gl_GlobalInvocationID.x];
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
			vk::DescriptorBufferInfo{keys.buffer(), 0, keys.deviceSize()},
			vk::DescriptorBufferInfo{prefixSums.buffer(), 0, prefixSums.deviceSize()},
			vk::DescriptorBufferInfo{prescanBlockSum.buffer(), 0, prescanBlockSum.deviceSize()}
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
			vk::DescriptorBufferInfo{blockSums[i].buffer(), 0, blockSums[i].deviceSize()},
			vk::DescriptorBufferInfo{blockSums[i + 1].buffer(), 0, blockSums[i + 1].deviceSize()}
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
			vk::DescriptorBufferInfo{keys.buffer(), 0, keys.deviceSize()},
			vk::DescriptorBufferInfo{prefixSums.buffer(), 0, prefixSums.deviceSize()},
			vk::DescriptorBufferInfo{prescanBlockSum.buffer(), 0, prescanBlockSum.deviceSize()},
			vk::DescriptorBufferInfo{result.buffer(), 0, result.deviceSize()},
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
				.size = hostBuffer.deviceSize()
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
			const uint32_t numBlockSums = (prefixSum.size() + blockSize - 1) / blockSize;

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

		for (ssize_t i = blockSums.size() - 2; i > 0; i--) {
			auto& prefixSum = blockSums[i - 1];
			const uint32_t numBlockSums = (prefixSum.size() + blockSize - 1) / blockSize;

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
				.size = result.deviceSize()
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
				.size = hostBuffer.deviceSize()
			}
		});

		buf.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eHost, {}, {
			vk::MemoryBarrier{
				.srcAccessMask = vk::AccessFlagBits::eTransferWrite|vk::AccessFlagBits::eMemoryWrite,
				.dstAccessMask = vk::AccessFlagBits::eHostRead|vk::AccessFlagBits::eMemoryRead
			}
		}, {}, {});
	});

	std::ranges::sort(initialKeys);
	REQUIRE_THAT(std::span(hostBuffer.data(), hostBuffer.size()), EqualsRange(initialKeys));
}

TEST_CASE_METHOD(ComputeShaderUnitTest, "Compute Shader Sort Test", "[sort] ")
{
	const size_t maxParticleCount = 128*128*128;
	const uint32_t sortBitCount = 32; // sort up to # of bits
	const uint32_t blockSize = 256;

	std::vector<uint32_t> initialKeys;

	{
		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_int_distribution<uint32_t> distrib(0, std::numeric_limits<uint32_t>::max());
		for (size_t i = 0; i < std::uniform_int_distribution<size_t>(0, maxParticleCount)(gen); ++i)
			initialKeys.emplace_back(distrib(gen));
	}


	if (initialKeys.size() % blockSize)
	{
		initialKeys.reserve(roundToNextMultiple(initialKeys.size(), blockSize));
		while (initialKeys.size() % blockSize)
			initialKeys.emplace_back(std::numeric_limits<uint32_t>::max());
	}

	const auto numKeys = static_cast<uint32_t>(initialKeys.size());


	using namespace pbf;
	descriptors::DescriptorSetLayout keyAndGlobalSortDescriptorSet{
		.createFlags = {},
		.bindings = {
			{
				.binding = 0,
				.descriptorType = vk::DescriptorType::eStorageBuffer,
				.descriptorCount = 1,
				.stageFlags = vk::ShaderStageFlagBits::eCompute
			},
			{
				.binding = 1,
				.descriptorType = vk::DescriptorType::eStorageBuffer,
				.descriptorCount = 1,
				.stageFlags = vk::ShaderStageFlagBits::eCompute
			}
		}
	};
	RadixSort radixSort{
		*this,
		blockSize,
		numKeys / blockSize,
		keyAndGlobalSortDescriptorSet,
		"shaders/unittestsort"
	};

	vk::UniqueDescriptorPool descriptorPool;
	{
		std::uint32_t numDescriptorSets = 64;
		static std::array<vk::DescriptorPoolSize, 1> sizes {{{ vk::DescriptorType::eStorageBuffer, 64 }}};
		descriptorPool = device().createDescriptorPoolUnique(vk::DescriptorPoolCreateInfo{
			.maxSets = numDescriptorSets,
			.poolSizeCount = static_cast<std::uint32_t>(sizes.size()),
			.pPoolSizes = sizes.data()
		});
	}


	Buffer<uint32_t> keysBuffer{
		*this,
		numKeys,
		vk::BufferUsageFlagBits::eStorageBuffer,
		MemoryType::DYNAMIC
	};
	Buffer<uint32_t> resultBuffer{
		*this,
		numKeys,
		vk::BufferUsageFlagBits::eStorageBuffer,
		MemoryType::DYNAMIC
	};

	{
		auto ptr = keysBuffer.data();
		for (auto i = 0; i < numKeys; ++i) {
			ptr[i] = initialKeys[i];
		}
	}

	vk::DescriptorSet pingDescriptorSet, pongDescriptorSet;
	{
		std::vector pingPongLayouts(2, *cache().fetch(keyAndGlobalSortDescriptorSet));
		auto descriptorSets = device().allocateDescriptorSets(vk::DescriptorSetAllocateInfo{
			.descriptorPool = *descriptorPool,
			.descriptorSetCount = size32(pingPongLayouts),
			.pSetLayouts = pingPongLayouts.data()
		});
		pingDescriptorSet = descriptorSets.front();
		pongDescriptorSet = descriptorSets.back();
	}

	{
		auto descriptorInfos = {
			vk::DescriptorBufferInfo{keysBuffer.buffer(), 0, keysBuffer.deviceSize()},
			vk::DescriptorBufferInfo{resultBuffer.buffer(), 0, resultBuffer.deviceSize()}
		};
		device().updateDescriptorSets({vk::WriteDescriptorSet{
			.dstSet = pingDescriptorSet,
			.dstBinding = 0,
			.dstArrayElement = 0,
			.descriptorCount = static_cast<uint32_t>(descriptorInfos.size()),
			.descriptorType = vk::DescriptorType::eStorageBuffer,
			.pImageInfo = nullptr,
			.pBufferInfo = &*descriptorInfos.begin(),
			.pTexelBufferView = nullptr
		}}, {});
	}
	{
		auto descriptorInfos = {
			vk::DescriptorBufferInfo{resultBuffer.buffer(), 0, resultBuffer.deviceSize()},
			vk::DescriptorBufferInfo{keysBuffer.buffer(), 0, keysBuffer.deviceSize()}
		};
		device().updateDescriptorSets({vk::WriteDescriptorSet{
			.dstSet = pongDescriptorSet,
			.dstBinding = 0,
			.dstArrayElement = 0,
			.descriptorCount = static_cast<uint32_t>(descriptorInfos.size()),
			.descriptorType = vk::DescriptorType::eStorageBuffer,
			.pImageInfo = nullptr,
			.pBufferInfo = &*descriptorInfos.begin(),
			.pTexelBufferView = nullptr
		}}, {});
	}

	bool swapped;
	run([&](vk::CommandBuffer buf) {
		swapped = radixSort.stage(buf, sortBitCount, pingDescriptorSet, pongDescriptorSet);
	});

	{

		auto ptr = swapped ? resultBuffer.data() : keysBuffer.data();

		std::sort(initialKeys.begin(), initialKeys.end());
		for (auto i = 0; i < numKeys; ++i)
			CHECK(ptr[i] == initialKeys[i]);
	}

}