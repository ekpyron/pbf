#include "Simulation.h"
#include "Context.h"
#include "Renderer.h"
#include "Scene.h"
#include <pbf/descriptors/DescriptorSet.h>
#include <imgui.h>
#include <random>

namespace pbf {

namespace {

void initializeParticleData(ParticleData* data, size_t numParticles)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(-0.25f, 0.25f);
    size_t id = 0;
    auto edgeLength = std::ceil(std::cbrt(numParticles));
    [&](){
        for (int32_t x = 0; x < edgeLength; ++x)
        {
            for (int32_t y = 0; y < edgeLength; ++y)
            {
                for (int32_t z = 0; z < edgeLength; ++z, ++id)
                {
                    if (id >= numParticles)
                        return;
                    data[id].position = glm::vec3(x - 32, -63 + y, z - 32);
                    data[id].position += glm::vec3(dist(gen), dist(gen), dist(gen));
                    data[id].position *= 0.8f;
                    data[id].velocity = glm::vec3(0,0,0);
                    data[id].type = id > (numParticles / 2);
                }
            }
        }
    }();
    std::shuffle(data, data + numParticles, gen);
}

constexpr auto radixSortDescriptorSetLayoutDescriptors() {
	return std::vector<descriptors::DescriptorSetLayout>{descriptors::DescriptorSetLayout{
		.createFlags = {},
		.bindings = {{
						 .binding = 0,
						 .descriptorType = vk::DescriptorType::eStorageBuffer,
						 .descriptorCount = 1,
						 .stageFlags = vk::ShaderStageFlagBits::eCompute
					 },{
						 .binding = 1,
						 .descriptorType = vk::DescriptorType::eUniformBuffer,
						 .descriptorCount = 1,
						 .stageFlags = vk::ShaderStageFlagBits::eCompute
					 },{
						 .binding = 2,
						 .descriptorType = vk::DescriptorType::eStorageBuffer,
						 .descriptorCount = 1,
						 .stageFlags = vk::ShaderStageFlagBits::eCompute
					 }},
		PBF_DESC_DEBUG_NAME("global sort and hash Particle Key Layout")
	}};
}
}

Simulation::Simulation(InitContext &initContext, size_t numParticles):
UIControlled(initContext.context.gui()),
_context(initContext.context),
_particleData([&]() {
    auto& context = initContext.context;
    RingBuffer<ParticleData> particleData{
            context,
            numParticles,
            2,
            vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer,
            pbf::MemoryType::STATIC
    };

    auto& initBuffer = initContext.createInitData<Buffer<ParticleData>>(
            context, numParticles, vk::BufferUsageFlagBits::eTransferSrc, pbf::MemoryType::TRANSIENT
    );

    ParticleData* data = initBuffer.data();
    initializeParticleData(data, numParticles);
    initBuffer.flush();

    auto& initCmdBuf = *initContext.initCommandBuffer;
    for (size_t i = 0; i < particleData.segments(); ++i)
        initCmdBuf.copyBuffer(initBuffer.buffer(), particleData.buffer(), {
                vk::BufferCopy {
                        .srcOffset = 0,
                        .dstOffset = sizeof(ParticleData) * numParticles * i,
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
_particleKeys(initContext.context, _particleData.size(), _particleData.segments(), vk::BufferUsageFlagBits::eStorageBuffer|vk::BufferUsageFlagBits::eTransferDst, MemoryType::STATIC),
_gridDataBuffer(_context, 1, vk::BufferUsageFlagBits::eUniformBuffer|vk::BufferUsageFlagBits::eTransferDst, MemoryType::STATIC),
_lambdaBuffer(initContext.context, _particleData.size(), vk::BufferUsageFlagBits::eStorageBuffer, MemoryType::STATIC),
_vorticityBuffer(initContext.context, _particleData.size(), vk::BufferUsageFlagBits::eStorageBuffer, MemoryType::STATIC),
_radixSort(_context, blockSize, getNumParticles() / blockSize, radixSortDescriptorSetLayoutDescriptors(), "shaders/particlesort"),
_neighbourCellFinder(_context, GridData{}.numCells(), getNumParticles()),
_tempBuffer(_context, _particleData.size(), 2, vk::BufferUsageFlagBits::eStorageBuffer|vk::BufferUsageFlagBits::eTransferSrc, MemoryType::STATIC)
{
	if (getNumParticles() % blockSize)
		throw std::runtime_error("Particle count must be a multiple of blockSize.");
	if (!getNumParticles())
		throw std::runtime_error("Need at least block size particles.");

	{
		auto& gridDataInitBuffer = initContext.createInitData<Buffer<GridData>>(
			_context,
			1,
			vk::BufferUsageFlagBits::eTransferSrc,
			MemoryType::TRANSIENT
		);
		std::construct_at<GridData>(gridDataInitBuffer.data());
		gridDataInitBuffer.flush();

		auto& initCmdBuf = *initContext.initCommandBuffer;

		initCmdBuf.pipelineBarrier(
			vk::PipelineStageFlagBits::eHost,
			vk::PipelineStageFlagBits::eTransfer,
			{},
			{
				vk::MemoryBarrier{
					.srcAccessMask = vk::AccessFlagBits::eHostWrite,
					.dstAccessMask = vk::AccessFlagBits::eTransferRead
				}
			},
			{}, {}
		);

		initCmdBuf.copyBuffer(gridDataInitBuffer.buffer(), _gridDataBuffer.buffer(), {
			vk::BufferCopy {
				.srcOffset = 0,
				.dstOffset = 0,
				.size = sizeof(GridData)
			}
		});

		initCmdBuf.pipelineBarrier(
			vk::PipelineStageFlagBits::eTransfer,
			vk::PipelineStageFlagBits::eComputeShader,
			{},
			{
				vk::MemoryBarrier{
					.srcAccessMask = vk::AccessFlagBits::eTransferWrite,
					.dstAccessMask = vk::AccessFlagBits::eUniformRead
				}
			},
			{}, {}
		);
	}


	initKeys(initContext.context, *initContext.initCommandBuffer);

	buildPipelines();
}

void Simulation::reset(vk::CommandBuffer &buf) {
    auto& initBuffer = _context.renderer().createFrameData<Buffer<ParticleData>>(
            _context, _particleData.size(), vk::BufferUsageFlagBits::eTransferSrc, pbf::MemoryType::TRANSIENT
    );
    ParticleData* data = initBuffer.data();

    initializeParticleData(data, _particleData.size());
    initBuffer.flush();

    for (size_t i = 0; i < _particleData.segments(); ++i)
        buf.copyBuffer(initBuffer.buffer(), _particleData.buffer(), {
                vk::BufferCopy {
                        .srcOffset = 0,
                        .dstOffset = _particleData.itemSize() * _particleData.size() * i,
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
}

std::string Simulation::uiCategory() const
{
	return "Simulation";
}

void Simulation::ui()
{
	bool rebuildPipelines = false;
	rebuildPipelines |= ImGui::SliderFloat("h", &h, 0.25f, 4.0f, "%.3f");
	rebuildPipelines |= ImGui::SliderFloat("rho_0", &rho_0, 0.1f, 10.0f, "%.1f");
	rebuildPipelines |= ImGui::SliderFloat("epsilon", &epsilon, 0.01f, 100.0f, "%.2f");
	rebuildPipelines |= ImGui::SliderFloat("xsph_viscosity_c", &xsph_viscosity_c, 0.0001f, 1.0f, "%.5f");
	rebuildPipelines |= ImGui::SliderFloat("tensile_instability_k", &tensile_instability_k, 0.01f, 1.0f, "%.3f");
	rebuildPipelines |= ImGui::SliderFloat("vorticity_epsilon", &vorticity_epsilon, 0.01f, 10.0f, "%.2f");
	if (rebuildPipelines)
		buildPipelines();
}

descriptors::ShaderStage::SpecializationInfo Simulation::makeSpecializationInfo() const
{
	// TODO: enforce alignment with common.comp
	auto Wpoly6 = [&](float r) -> float {
		if (r > h)
			return 0;
		float tmp = h * h - r * r;
		return 1.56668147106 * tmp * tmp * tmp / (h*h*h*h*h*h*h*h*h);
	};
	float tensile_instability_scale = 1.0f / Wpoly6 (0.2f);
	return {
		Specialization<uint32_t>{.constantID = 0, .value = blockSize},
		Specialization<float>{.constantID = 1, .value = h},
		Specialization<float>{.constantID = 2, .value = 1.0f / rho_0},
		Specialization<float>{.constantID = 3, .value = epsilon},
		Specialization<float>{.constantID = 4, .value = xsph_viscosity_c},
		Specialization<float>{.constantID = 5, .value = tensile_instability_k},
		Specialization<float>{.constantID = 6, .value = vorticity_epsilon},
		Specialization<float>{.constantID = 7, .value = tensile_instability_scale},
	};
}


void Simulation::initKeys(Context& context, vk::CommandBuffer buf)
{
	Cache& cache = context.cache();
	{
		auto keyInitSetLayout = cache.fetch(descriptors::DescriptorSetLayout{
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
			},
			PBF_DESC_DEBUG_NAME("Simulation key init descriptor set layout")
		});
		auto keyInitPipelineLayout = cache.fetch(
			descriptors::PipelineLayout{
				.setLayouts = {keyInitSetLayout},
				PBF_DESC_DEBUG_NAME("Key init pipeline layout.")
			});
		auto keyInitPipeline = cache.fetch(
		descriptors::ComputePipeline{
			.flags = {},
			.shaderStage = descriptors::ShaderStage {
				.stage = vk::ShaderStageFlagBits::eCompute,
				.module = cache.fetch(
					descriptors::ShaderModule{
						.source = descriptors::ShaderModule::File{"shaders/simulation/keyinit.comp.spv"},
						PBF_DESC_DEBUG_NAME("Simulation: key init shader module")
					}),
				.entryPoint = "main",
				.specialization = {
					Specialization<uint32_t>{.constantID = 0, .value = blockSize}
				}
			},
			.pipelineLayout = keyInitPipelineLayout,
			PBF_DESC_DEBUG_NAME("Simulation: key init shader pipeline")
		}
		);
		for (size_t i = 0; i < _particleData.segments(); ++i) {
			_context.bindPipeline(
				buf, keyInitPipeline,
				{{_particleData.segment(i), _particleKeys.segment(i)}}
			);
			buf.dispatch(((getNumParticles() + blockSize - 1) / blockSize), 1, 1);
		}

		buf.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader, {}, {
			vk::MemoryBarrier{
				.srcAccessMask = vk::AccessFlagBits::eShaderWrite,
				.dstAccessMask = vk::AccessFlagBits::eShaderRead
			}
		}, {}, {});
	}
}

// currentFrameSync (readonly) -> nextFrameSync (writeonly)
void Simulation::run(vk::CommandBuffer buf, float timestep)
{
	if (_resetKeys)
	{
		initKeys(_context, buf);
		_resetKeys = false;
	}
	_context.bindPipeline(buf, _unconstrainedSystemUpdatePipeline, {
		{_particleKeys.segment(ringBufferIndex)},
		{_particleData.segment(ringBufferIndex)}
	});
	static constexpr float Gabs = 9.81f;
	UnconstrainedPositionUpdatePushConstants pushConstants{
		.externalAccell = glm::vec3(0.0f, -Gabs, 0.0f),
		.lastTimestep = _lastTimestep,
		.timestep = timestep
	};
	_lastTimestep = timestep;
	pushConstants.externalAccell += glm::vec3(_context.window().getKey(GLFW_KEY_LEFT) ? 0.5f * Gabs : 0.0f, 0, _context.window().getKey(GLFW_KEY_UP) ? 0.5f * Gabs : 0.0f);
	pushConstants.externalAccell += glm::vec3(_context.window().getKey(GLFW_KEY_RIGHT) ? -0.5f * Gabs : 0.0f, 0, _context.window().getKey(GLFW_KEY_DOWN) ? -0.5f * Gabs : 0.0f);

	buf.pushConstants(*(_unconstrainedSystemUpdatePipeline.descriptor().pipelineLayout), vk::ShaderStageFlagBits::eCompute, 0, sizeof(pushConstants), &pushConstants);

	buf.dispatch(((getNumParticles() + blockSize - 1) / blockSize), 1, 1);

	buf.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader, {}, {
		vk::MemoryBarrier{
			.srcAccessMask = vk::AccessFlagBits::eShaderWrite,
			.dstAccessMask = vk::AccessFlagBits::eShaderRead
		}
	}, {}, {});



	std::vector<descriptors::DescriptorSetBinding> initInfos {
		_particleKeys.segment(ringBufferIndex),
		_gridDataBuffer.fullBufferInfo(),
		_tempBuffer.segment(1)
	};
	std::vector<descriptors::DescriptorSetBinding> pingInfos {
		_tempBuffer.segment(0),
		_gridDataBuffer.fullBufferInfo(),
		_tempBuffer.segment(1)
	};
	std::vector<descriptors::DescriptorSetBinding> pongInfos {
		_tempBuffer.segment(1),
		_gridDataBuffer.fullBufferInfo(),
		_tempBuffer.segment(0)
	};

	static constexpr size_t numSteps = 3;
	for (size_t step = 0; step < numSteps; ++step) {
        // TODO: adjust neighbourCellFinderInputInfos according to expected sortResult
        auto sortResult = _radixSort.stage(
                buf,
                30,
                step == 0 ? initInfos : pingInfos, pingInfos, pongInfos
        );

        size_t pingBufferSegment = sortResult == RadixSort::Result::InPingBuffer ? 0 : 1;
        size_t pongBufferSegment = sortResult == RadixSort::Result::InPingBuffer ? 1 : 0;

        _neighbourCellFinder(buf, _particleData.size(), _tempBuffer.segment(pingBufferSegment), _gridDataBuffer.fullBufferInfo());



        bool isLast = step == (numSteps - 1);
		_context.bindPipeline(
			buf, _calcLambdaPipeline,
			{
				{_tempBuffer.segment(pingBufferSegment), _gridDataBuffer.fullBufferInfo()},
				{_neighbourCellFinder.gridBoundaryBuffer().fullBufferInfo()},
				{_lambdaBuffer.fullBufferInfo()},
				{_particleData.segment(nextRingBufferIndex())}
			}
		);
		buf.dispatch(((getNumParticles() + blockSize - 1) / blockSize), 1, 1);

		buf.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader, {}, {
			vk::MemoryBarrier{
				.srcAccessMask = vk::AccessFlagBits::eShaderWrite,
				.dstAccessMask = vk::AccessFlagBits::eShaderRead
			}
		}, {}, {});


		_context.bindPipeline(buf, _updatePosPipeline,
					 {
						 { // set 0
							 _tempBuffer.segment(pingBufferSegment),
							 _gridDataBuffer.fullBufferInfo()
						 },
						 { // set 1
							 _neighbourCellFinder.gridBoundaryBuffer().fullBufferInfo()
						 },
						 { // set 2
							 _lambdaBuffer.fullBufferInfo()
						 },
						 { // set 3
							 isLast ? _particleKeys.segment(nextRingBufferIndex())
							 		: _tempBuffer.segment(pongBufferSegment)
						 },
						 {_particleData.segment(nextRingBufferIndex())}
					 });
		buf.dispatch(((getNumParticles() + blockSize - 1) / blockSize), 1, 1);

		buf.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader, {}, {
			vk::MemoryBarrier{
				.srcAccessMask = vk::AccessFlagBits::eShaderWrite,
				.dstAccessMask = vk::AccessFlagBits::eShaderRead
			}
		}, {}, {});

		std::swap(pingBufferSegment, pongBufferSegment);
	}

	_context.bindPipeline(
		buf,
		_particleDataUpdatePipeline,
		{
			{_particleKeys.segment(nextRingBufferIndex())},
			{_particleData.segment(nextRingBufferIndex())},
			{_particleData.segment(ringBufferIndex)}
		}
	);
	buf.pushConstants(*(_particleDataUpdatePipeline.descriptor().pipelineLayout), vk::ShaderStageFlagBits::eCompute, 0, sizeof(float), &timestep);
	buf.dispatch(((getNumParticles() + blockSize - 1) / blockSize), 1, 1);

	buf.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader, {}, {
		vk::MemoryBarrier{
			.srcAccessMask = vk::AccessFlagBits::eShaderWrite,
			.dstAccessMask = vk::AccessFlagBits::eShaderRead
		}
	}, {}, {});

	_context.bindPipeline(
		buf,
		_calcVorticityPipeline,
		{
			{_particleKeys.segment(nextRingBufferIndex()), _gridDataBuffer.fullBufferInfo()},
			{_neighbourCellFinder.gridBoundaryBuffer().fullBufferInfo()},
			{_vorticityBuffer.fullBufferInfo()},
			{_particleData.segment(nextRingBufferIndex())},
			{_particleData.segment(ringBufferIndex)},
		}
	);
	buf.dispatch(((getNumParticles() + blockSize - 1) / blockSize), 1, 1);

	buf.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader, {}, {
		vk::MemoryBarrier{
			.srcAccessMask = vk::AccessFlagBits::eShaderWrite,
			.dstAccessMask = vk::AccessFlagBits::eShaderRead
		}
	}, {}, {});

	_context.bindPipeline(
		buf,
		_updateVelPipeline,
		{
			{_particleKeys.segment(nextRingBufferIndex()), _gridDataBuffer.fullBufferInfo()},
			{_neighbourCellFinder.gridBoundaryBuffer().fullBufferInfo()},
			{_vorticityBuffer.fullBufferInfo()},
			{_particleData.segment(ringBufferIndex)},
			{_particleData.segment(nextRingBufferIndex())}
		}
	);
	buf.pushConstants(*(_updateVelPipeline.descriptor().pipelineLayout), vk::ShaderStageFlagBits::eCompute, 0, sizeof(float), &timestep);
	buf.dispatch(((getNumParticles() + blockSize - 1) / blockSize), 1, 1);


	buf.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader, {}, {
		vk::MemoryBarrier{
			.srcAccessMask = vk::AccessFlagBits::eShaderWrite,
			.dstAccessMask = vk::AccessFlagBits::eShaderRead
		}
	}, {}, {});

    ringBufferIndex = nextRingBufferIndex();
}

void Simulation::copy(vk::CommandBuffer buf, vk::Buffer dst, size_t dstOffset)
{
    size_t srcOffset = _particleData.itemSize() * _particleData.size() * previousRingBufferIndex();
    buf.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eTransfer, {}, {}, {vk::BufferMemoryBarrier{
            .srcAccessMask = vk::AccessFlagBits::eShaderWrite,
            .dstAccessMask = vk::AccessFlagBits::eTransferRead,
            .srcQueueFamilyIndex = 0,
            .dstQueueFamilyIndex = 0,
            .buffer = _particleData.buffer(),
            .offset = srcOffset,
            .size = _particleData.segmentDeviceSize(),
    }}, {});

    buf.copyBuffer(_particleData.buffer(), dst, {
            vk::BufferCopy {
                    .srcOffset = srcOffset,
                    .dstOffset = dstOffset,
                    .size = _particleData.segmentDeviceSize()
            }
    });

    // TODO: move outside of this function to usage sites
    buf.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eVertexInput, {}, {}, {vk::BufferMemoryBarrier{
            .srcAccessMask = vk::AccessFlagBits::eTransferWrite,
            .dstAccessMask = vk::AccessFlagBits::eVertexAttributeRead,
            .srcQueueFamilyIndex = 0,
            .dstQueueFamilyIndex = 0,
            .buffer = dst,
            .offset = dstOffset,
            .size = _particleData.segmentDeviceSize(),
    }}, {});
}
void Simulation::buildPipelines()
{
	descriptors::ShaderStage::SpecializationInfo specializationInfo = makeSpecializationInfo();

	auto singleStorageBufferDescriptorSetLayout = _context.cache().fetch(descriptors::DescriptorSetLayout{
		.createFlags = {},
		.bindings = {
			{
				.binding = 0,
				.descriptorType = vk::DescriptorType::eStorageBuffer,
				.descriptorCount = 1,
				.stageFlags = vk::ShaderStageFlagBits::eCompute
			}
		},
		PBF_DESC_DEBUG_NAME("Simulation: single storage buffer descriptor set")
	});

	auto& cache = _context.cache();
	{

		auto particleDataUpdatePipelineLayout = cache.fetch(
			descriptors::PipelineLayout{
				.setLayouts = {singleStorageBufferDescriptorSetLayout, singleStorageBufferDescriptorSetLayout, singleStorageBufferDescriptorSetLayout},
				.pushConstants = {
					vk::PushConstantRange{
						vk::ShaderStageFlagBits::eCompute,
						0,
						sizeof(float)
					}
				},
				PBF_DESC_DEBUG_NAME("Particle data update pipeline Layout")
			});
		_particleDataUpdatePipeline = cache.fetch(
			descriptors::ComputePipeline{
				.flags = {},
				.shaderStage = descriptors::ShaderStage {
					.stage = vk::ShaderStageFlagBits::eCompute,
					.module = cache.fetch(
					descriptors::ShaderModule{
						.source = descriptors::ShaderModule::File{"shaders/simulation/particledataupdate.comp.spv"},
						PBF_DESC_DEBUG_NAME("Simulation: particle data update shader module")
					}),
					.entryPoint = "main",
					.specialization = specializationInfo
				},
				.pipelineLayout = particleDataUpdatePipelineLayout,
				PBF_DESC_DEBUG_NAME("Simulation: particle data update shader pipeline")
			}
		);
	}
	{
		auto unconstrainedSystemUpdatePipelineLayout = cache.fetch(
			descriptors::PipelineLayout{
				.setLayouts = {singleStorageBufferDescriptorSetLayout, singleStorageBufferDescriptorSetLayout},
				.pushConstants = {
					vk::PushConstantRange{
						vk::ShaderStageFlagBits::eCompute,
						0,
						sizeof(UnconstrainedPositionUpdatePushConstants)
					}
				},
				PBF_DESC_DEBUG_NAME("Unconstrained update pipeline Layout")
			});
		_unconstrainedSystemUpdatePipeline = cache.fetch(
			descriptors::ComputePipeline{
				.flags = {},
				.shaderStage = descriptors::ShaderStage {
					.stage = vk::ShaderStageFlagBits::eCompute,
					.module = cache.fetch(
					descriptors::ShaderModule{
						.source = descriptors::ShaderModule::File{"shaders/simulation/unconstrainedupdate.comp.spv"},
						PBF_DESC_DEBUG_NAME("Simulation: unconstrained system update shader (Update positions based on velocity and external forces without considering constraint violations.)")
					}),
					.entryPoint = "main",
					.specialization = specializationInfo
				},
				.pipelineLayout = unconstrainedSystemUpdatePipelineLayout,
				PBF_DESC_DEBUG_NAME("Simulation: unconstrained system update pipeline (Update positions based on velocity and external forces without considering constraint violations.)")
			}
		);
	}

	{
		auto inputDescriptorSetLayout = cache.fetch(descriptors::DescriptorSetLayout{
			.createFlags = {},
			.bindings = {{
							 .binding = 0,
							 .descriptorType = vk::DescriptorType::eStorageBuffer,
							 .descriptorCount = 1,
							 .stageFlags = vk::ShaderStageFlagBits::eCompute
						 },{
							 .binding = 1,
							 .descriptorType = vk::DescriptorType::eUniformBuffer,
							 .descriptorCount = 1,
							 .stageFlags = vk::ShaderStageFlagBits::eCompute
						 }},
			PBF_DESC_DEBUG_NAME("One storage buffer one uniform buffer descriptor set layout")
		});
		auto gridDataDescriptorSetLayout = cache.fetch(descriptors::DescriptorSetLayout{
			.createFlags = {},
			.bindings = {{
				.binding = 0,
				.descriptorType = vk::DescriptorType::eStorageBuffer,
				.descriptorCount = 1,
				.stageFlags = vk::ShaderStageFlagBits::eCompute
			}},
			PBF_DESC_DEBUG_NAME("Grid Data Layout")
		});
		auto calcLambdaPipelineLayout = cache.fetch(
			descriptors::PipelineLayout{
				.setLayouts = {inputDescriptorSetLayout, gridDataDescriptorSetLayout,
					singleStorageBufferDescriptorSetLayout /* lambda */,
					singleStorageBufferDescriptorSetLayout /* vorticity */,
					singleStorageBufferDescriptorSetLayout /* particle data */,
					singleStorageBufferDescriptorSetLayout /* next particle data */},
				PBF_DESC_DEBUG_NAME("Calc lambda pipeline Layout")
			});

		_calcLambdaPipeline = cache.fetch(
			descriptors::ComputePipeline{
				.flags = {},
				.shaderStage = descriptors::ShaderStage {
					.stage = vk::ShaderStageFlagBits::eCompute,
					.module = cache.fetch(
					descriptors::ShaderModule{
						.source = descriptors::ShaderModule::File{"shaders/simulation/calclambda.comp.spv"},
						PBF_DESC_DEBUG_NAME("Simulation: Calc Lambda Shader")
					}),
					.entryPoint = "main",
					.specialization = specializationInfo
				},
				.pipelineLayout = calcLambdaPipelineLayout,
				PBF_DESC_DEBUG_NAME("Simulation: calc lambda pipeline")
			}
		);

		auto calcVorticityPipelineLayout = cache.fetch(
			descriptors::PipelineLayout{
				.setLayouts = {inputDescriptorSetLayout, gridDataDescriptorSetLayout,
					singleStorageBufferDescriptorSetLayout /* vorticity */,
					singleStorageBufferDescriptorSetLayout /* particle data */,
					singleStorageBufferDescriptorSetLayout /* next particle data */},
				PBF_DESC_DEBUG_NAME("Calc lambda pipeline Layout")
			});

		_calcVorticityPipeline = cache.fetch(
			descriptors::ComputePipeline{
				.flags = {},
				.shaderStage = descriptors::ShaderStage {
					.stage = vk::ShaderStageFlagBits::eCompute,
					.module = cache.fetch(
					descriptors::ShaderModule{
						.source = descriptors::ShaderModule::File{"shaders/simulation/calcvorticity.comp.spv"},
						PBF_DESC_DEBUG_NAME("Simulation: Calc Vorticity Shader")
					}),
					.entryPoint = "main",
					.specialization = specializationInfo
				},
				.pipelineLayout = calcVorticityPipelineLayout,
				PBF_DESC_DEBUG_NAME("Simulation: calc vorticity pipeline")
			}
		);

		auto updateVelPipelineLayout = cache.fetch(
			descriptors::PipelineLayout{
				.setLayouts = {inputDescriptorSetLayout, gridDataDescriptorSetLayout,
					singleStorageBufferDescriptorSetLayout /* lambda */,
					singleStorageBufferDescriptorSetLayout /* key output */,
					singleStorageBufferDescriptorSetLayout /* vorticities */,
					singleStorageBufferDescriptorSetLayout /* particle data in */,
					singleStorageBufferDescriptorSetLayout /* particle data out */
				},
				.pushConstants = {
					vk::PushConstantRange{
						vk::ShaderStageFlagBits::eCompute,
						0,
						sizeof(float)
					}
				},
				PBF_DESC_DEBUG_NAME("Calc lambda pipeline Layout")
			});

		_updateVelPipeline = cache.fetch(
			descriptors::ComputePipeline{
				.flags = {},
				.shaderStage = descriptors::ShaderStage {
					.stage = vk::ShaderStageFlagBits::eCompute,
					.module = cache.fetch(
					descriptors::ShaderModule{
						.source = descriptors::ShaderModule::File{"shaders/simulation/updatevel.comp.spv"},
						PBF_DESC_DEBUG_NAME("Simulation: Update Vel Shader")
					}),
					.entryPoint = "main",
					.specialization = specializationInfo
				},
				.pipelineLayout = updateVelPipelineLayout,
				PBF_DESC_DEBUG_NAME("Simulation: update vel pipeline")
			}
		);


		auto updatePosPipelineLayout = cache.fetch(
			descriptors::PipelineLayout{
				.setLayouts = {inputDescriptorSetLayout, gridDataDescriptorSetLayout,
					singleStorageBufferDescriptorSetLayout /* lambda */,
					singleStorageBufferDescriptorSetLayout /* key output */,
					singleStorageBufferDescriptorSetLayout /* vorticities */,
					singleStorageBufferDescriptorSetLayout /* particle data in */,
					singleStorageBufferDescriptorSetLayout /* particle data out */
				},
				PBF_DESC_DEBUG_NAME("Calc lambda pipeline Layout")
			});

		_updatePosPipeline = cache.fetch(
			descriptors::ComputePipeline{
				.flags = {},
				.shaderStage = descriptors::ShaderStage {
					.stage = vk::ShaderStageFlagBits::eCompute,
					.module = cache.fetch(
					descriptors::ShaderModule{
						.source = descriptors::ShaderModule::File{"shaders/simulation/updatepos.comp.spv"},
						PBF_DESC_DEBUG_NAME("Simulation: Update Pos Shader")
					}),
					.entryPoint = "main",
					.specialization = specializationInfo
				},
				.pipelineLayout = updatePosPipelineLayout,
				PBF_DESC_DEBUG_NAME("Simulation: update pos pipeline")
			}
		);
	}
}

}
