/**
 *
 *
 * @file Scene.cpp
 * @brief 
 * @author clonker
 * @date 1/23/19
 */
#include <cstdint>
#include <random>

#include "Scene.h"
#include "Renderer.h"
#include "Simulation.h"

#include "descriptors/DescriptorSetLayout.h"
#include "IndirectCommandsBuffer.h"

namespace pbf {

void initializeParticleData(ParticleData* data, size_t numParticles)
{
	std::random_device rd;  //Will be used to obtain a seed for the random number engine
	std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
	std::uniform_real_distribution<float> dist(-0.25f, 0.25f);

	struct Constraint {
		uint32_t j = 0;
		float dist = 0.0f;
	};

	glm::ivec3 offsets[6] = {
		glm::ivec3(1, 0, 0),
		glm::ivec3(-1, 0, 0),
		glm::ivec3(0, 1, 0),
		glm::ivec3(0, -1, 0),
		glm::ivec3(0, 0, 1),
		glm::ivec3(0, 0, -1)
	};

	auto cubeSide = std::floor(std::cbrt(numParticles / 2));
	auto linearize = [&](glm::ivec3 const& _v) {
		return _v.x * cubeSide * cubeSide + _v.y * cubeSide + _v.z;
	};
	auto packConstraints = [](Constraint const* src, glm::vec4* dst) {
		size_t dstIndex = 0;
		size_t dstComponent = 0;
		auto pushBack = [&](uint32_t j, float dist) {
			dst[dstIndex][dstComponent++] = *reinterpret_cast<float*>(&j);
			dst[dstIndex][dstComponent++] = dist;
			if (dstComponent >= 4) {
				dstComponent = 0;
				++dstIndex;
			}
		};

		for (int32_t i = 0; i < 6; ++i)
			pushBack(src[i].j, src[i].dist);
	};
	for (int32_t x = 0; x < cubeSide; ++x)
		for (int32_t y = 0; y < cubeSide; ++y)
			for (int32_t z = 0; z < cubeSide; ++z)
			{
				glm::ivec3 ix3 (x,y,z);
				int32_t id = linearize(ix3);
				data[id].position = glm::vec3(x - 32, -63 + y, z - 32);
				data[id].velocity = glm::vec3(0,0,0);
				data[id].type = 0;

				Constraint constraints[6];
				for(int32_t i = 0; i < 6; ++i)
				{
					glm::ivec3 jx3 = ix3 + offsets[i];
					if (jx3.x >= 0 && jx3.y >= 0 && jx3.z >= 0 &&
						jx3.x < cubeSide && jx3.y < cubeSide && jx3.z < cubeSide)
					{
						constraints[i].dist = 1.0f;
						constraints[i].j = linearize(jx3);
					}
					else
						constraints[i] = {};
				}
				packConstraints(constraints, data[id].packedData);
			}

	size_t id = cubeSide * cubeSide * cubeSide;

	auto edgeLength = std::ceil(std::cbrt(numParticles / 2));
	[&](){
		for (int32_t x = 0; x < edgeLength; ++x)
		{
			for (int32_t y = 0; y < edgeLength; ++y)
			{
				for (int32_t z = 0; z < edgeLength; ++z, ++id)
				{
					if (id >= numParticles)
						return;
					data[id].position = glm::vec3(x - 32, -63 + y + cubeSide * 2, z - 32);
					data[id].position += glm::vec3(dist(gen), dist(gen), dist(gen));
					data[id].position *= 0.8f;
					data[id].velocity = glm::vec3(0,0,0);
					data[id].type = 1;
				}
			}
		}
	}();

	std::shuffle(data, data + numParticles, gen);
}

Scene::Scene(InitContext &initContext)
: _context(initContext.context),
_particleData([&](){
	auto& context = initContext.context;
	RingBuffer<ParticleData> particleData{
		context,
		_numParticles,
		context.renderer().framePrerenderCount(),
		// TODO: maybe remove transfer src
		vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eVertexBuffer,
		pbf::MemoryType::STATIC
	};

	auto& initBuffer = initContext.createInitData<Buffer<ParticleData>>(
		context, _numParticles, vk::BufferUsageFlagBits::eTransferSrc, pbf::MemoryType::TRANSIENT
	);

	ParticleData* data = initBuffer.data();
	initializeParticleData(data, _numParticles);
	initBuffer.flush();

	auto& initCmdBuf = *initContext.initCommandBuffer;
	for (size_t i = 0; i < context.renderer().framePrerenderCount(); ++i)
		initCmdBuf.copyBuffer(initBuffer.buffer(), particleData.buffer(), {
			vk::BufferCopy {
				.srcOffset = 0,
				.dstOffset = sizeof(ParticleData) * _numParticles * i,
				.size = initBuffer.deviceSize()
			}
		});

	initCmdBuf.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eComputeShader, {}, {}, {vk::BufferMemoryBarrier{
		.srcAccessMask = vk::AccessFlagBits::eTransferWrite,
		.dstAccessMask = vk::AccessFlagBits::eShaderRead,
		.srcQueueFamilyIndex = 0,
		.dstQueueFamilyIndex = 0,
		.buffer = particleData.buffer(),
		.offset = 0,
		.size = particleData.deviceSize(),
	}}, {});
	return particleData;
}()),
quad(initContext, *this),
_simulation(initContext, _particleData)
{
}

void Scene::resetParticles()
{
	_resetParticles = true;
	_simulation.resetKeys();
}

void Scene::frame(vk::CommandBuffer &buf) {
	if (_resetParticles)
	{
		auto& initBuffer = _context.renderer().createFrameData<Buffer<ParticleData>>(
			_context, _numParticles, vk::BufferUsageFlagBits::eTransferSrc, pbf::MemoryType::TRANSIENT
		);
		ParticleData* data = initBuffer.data();

		initializeParticleData(data, _numParticles);
		initBuffer.flush();

		for (size_t i = 0; i < _context.renderer().framePrerenderCount(); ++i)
			buf.copyBuffer(initBuffer.buffer(), _particleData.buffer(), {
				vk::BufferCopy {
					.srcOffset = 0,
					.dstOffset = sizeof(ParticleData) * _numParticles * i,
					.size = initBuffer.deviceSize()
				}
			});

		buf.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eComputeShader, {}, {}, {vk::BufferMemoryBarrier{
			.srcAccessMask = vk::AccessFlagBits::eTransferWrite,
			.dstAccessMask = vk::AccessFlagBits::eShaderRead,
			.srcQueueFamilyIndex = 0,
			.dstQueueFamilyIndex = 0,
			.buffer = _particleData.buffer(),
			.offset = 0,
			.size = _particleData.deviceSize(),
		}}, {});
		_resetParticles = false;
	}

	for(auto* ptr: indirectCommandBuffers) ptr->clear();

    quad.frame(_numParticles);
}

void Scene::enqueueCommands(vk::CommandBuffer &buf) {
    const auto &device = _context.device();

    for (auto& [graphicsPipeline, innerMap] : indirectDrawCalls)
    {
        buf.bindPipeline(vk::PipelineBindPoint::eGraphics, *graphicsPipeline);
        buf.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *(graphicsPipeline.descriptor().pipelineLayout), 0, { _context.globalDescriptorSet()}, {});

		for (auto& [pushConstantData, innerMap] : innerMap)
		{
			if (!pushConstantData.empty())
				buf.pushConstants(
					*(graphicsPipeline.descriptor().pipelineLayout),
					vk::ShaderStageFlagBits::eVertex,
					0,
					static_cast<uint32_t>(pushConstantData.size()),
					pushConstantData.data()
				);
			for (auto &[index, innerMap]: innerMap)
			{
				auto &[indexBuffer, indexType] = index;
				buf.bindIndexBuffer(indexBuffer.buffer->buffer(), 0, indexType);
				for (auto &[vertexBufferRefs, indirectCommandBuffer]: innerMap)
				{
					std::vector<vk::Buffer> vkBuffers;
					std::vector<vk::DeviceSize> offsets;
					for (BufferRef<Quad::VertexData> const& vertexBufferRef: vertexBufferRefs) {
						vkBuffers.emplace_back(vertexBufferRef.buffer->buffer());
						offsets.emplace_back(vertexBufferRef.offset);
					}
					buf.bindVertexBuffers(0, vkBuffers, offsets);

					{
						auto it = indirectCommandBuffer.buffers().begin();
						while (it != indirectCommandBuffer.buffers().end())
						{
							auto next = it;
							++next;

							auto lastBuffer = next == indirectCommandBuffer.buffers().end();
							buf.drawIndexedIndirect(
								it->buffer(), 0,
								lastBuffer ?
									indirectCommandBuffer.elementsInLastBuffer():
									IndirectCommandsBuffer::bufferSize, sizeof(vk::DrawIndirectCommand));
							it = next;
						}

					}
				}
			}
		}

    }

}


}
