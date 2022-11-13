#include "Simulation.h"
#include "Context.h"
#include "Renderer.h"
#include "Scene.h"
#include <pbf/descriptors/DescriptorSet.h>

namespace pbf {

namespace {
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

Simulation::Simulation(InitContext &initContext, RingBuffer<ParticleData>& particleData):
_context(initContext.context),
_particleData(particleData),
_particleKeys(initContext.context, particleData.size(), particleData.segments(), vk::BufferUsageFlagBits::eStorageBuffer|vk::BufferUsageFlagBits::eTransferDst, MemoryType::STATIC),
_gridDataBuffer(_context, 1, vk::BufferUsageFlagBits::eUniformBuffer|vk::BufferUsageFlagBits::eTransferDst, MemoryType::STATIC),
_lambdaBuffer(initContext.context, particleData.size(), vk::BufferUsageFlagBits::eStorageBuffer, MemoryType::STATIC),
_radixSort(_context, blockSize, getNumParticles() / blockSize, radixSortDescriptorSetLayoutDescriptors(), "shaders/particlesort"),
_neighbourCellFinder(_context, GridData{}.numCells(), getNumParticles()),
_tempBuffer(_context, particleData.size(), 2, vk::BufferUsageFlagBits::eStorageBuffer|vk::BufferUsageFlagBits::eTransferSrc, MemoryType::STATIC)
{
	if (getNumParticles() % blockSize)
		throw std::runtime_error("Particle count must be a multiple of blockSize.");
	if (!getNumParticles())
		throw std::runtime_error("Need at least block size particles.");

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

	auto& cache = _context.cache();
	{

		auto particleDataUpdatePipelineLayout = cache.fetch(
			descriptors::PipelineLayout{
				.setLayouts = {singleStorageBufferDescriptorSetLayout, singleStorageBufferDescriptorSetLayout},
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
					.specialization = {
						Specialization<uint32_t>{.constantID = 0, .value = blockSize}
					}
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
					.specialization = {
						Specialization<uint32_t>{.constantID = 0, .value = blockSize}
					}
				},
				.pipelineLayout = unconstrainedSystemUpdatePipelineLayout,
				PBF_DESC_DEBUG_NAME("Simulation: unconstrained system update pipeline (Update positions based on velocity and external forces without considering constraint violations.)")
			}
		);
	}

	initKeys(initContext.context, *initContext.initCommandBuffer);


	{
		auto& cache = _context.cache();

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
				.setLayouts = {inputDescriptorSetLayout, gridDataDescriptorSetLayout, singleStorageBufferDescriptorSetLayout /* lambda */},
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
					.specialization = {
						Specialization<uint32_t>{.constantID = 0, .value = blockSize}
					}
				},
				.pipelineLayout = calcLambdaPipelineLayout,
				PBF_DESC_DEBUG_NAME("Simulation: calc lambda pipeline")
			}
		);

		auto updatePosPipelineLayout = cache.fetch(
			descriptors::PipelineLayout{
				.setLayouts = {inputDescriptorSetLayout, gridDataDescriptorSetLayout, singleStorageBufferDescriptorSetLayout /* lambda */, singleStorageBufferDescriptorSetLayout /* key output */},
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
					.specialization = {
						Specialization<uint32_t>{.constantID = 0, .value = blockSize}
					}
				},
				.pipelineLayout = updatePosPipelineLayout,
				PBF_DESC_DEBUG_NAME("Simulation: update pos pipeline")
			}
		);
	}
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
		buf.bindPipeline(
			vk::PipelineBindPoint::eCompute,
			*cache.fetch(
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
			)
		);
		for (size_t i = 0; i < _context.renderer().framePrerenderCount(); ++i) {
			auto keyInitDescriptorSet = _context.device().allocateDescriptorSets(vk::DescriptorSetAllocateInfo{
				.descriptorPool = _context.descriptorPool(),
				.descriptorSetCount = 1,
				.pSetLayouts = &*keyInitSetLayout
			}).front();
			auto keyInitInfos = {
				_particleData.segment(i),
				_particleKeys.segment(i)
			};
			_context.device().updateDescriptorSets({vk::WriteDescriptorSet{
				.dstSet = keyInitDescriptorSet,
				.dstBinding = 0,
				.dstArrayElement = 0,
				.descriptorCount = 2,
				.descriptorType = vk::DescriptorType::eStorageBuffer,
				.pImageInfo = nullptr,
				.pBufferInfo = &*keyInitInfos.begin(),
				.pTexelBufferView = nullptr
			}}, {});
			buf.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *keyInitPipelineLayout, 0, {keyInitDescriptorSet}, {});
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
		{_particleKeys.segment(_context.renderer().currentFrameSync())},
		{_particleData.segment(_context.renderer().previousFrameSync())}
	});
	static constexpr float Gabs = 9.81f;
	UnconstrainedPositionUpdatePushConstants pushConstants{
		.externalForces = glm::vec3(0.0f, -Gabs, 0.0f),
		.lastTimestep = _lastTimestep,
		.timestep = timestep
	};
	_lastTimestep = timestep;
	pushConstants.externalForces += glm::vec3(_context.window().getKey(GLFW_KEY_LEFT) ? 0.5f * Gabs : 0.0f, 0, _context.window().getKey(GLFW_KEY_UP) ? 0.5f * Gabs : 0.0f);
	pushConstants.externalForces += glm::vec3(_context.window().getKey(GLFW_KEY_RIGHT) ? -0.5f * Gabs : 0.0f, 0, _context.window().getKey(GLFW_KEY_DOWN) ? -0.5f * Gabs : 0.0f);

	buf.pushConstants(*(_unconstrainedSystemUpdatePipeline.descriptor().pipelineLayout), vk::ShaderStageFlagBits::eCompute, 0, sizeof(pushConstants), &pushConstants);

	buf.dispatch(((getNumParticles() + blockSize - 1) / blockSize), 1, 1);

	buf.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader, {}, {
		vk::MemoryBarrier{
			.srcAccessMask = vk::AccessFlagBits::eShaderWrite,
			.dstAccessMask = vk::AccessFlagBits::eShaderRead
		}
	}, {}, {});



	std::vector<descriptors::DescriptorSetBinding> initInfos {
		_particleKeys.segment(_context.renderer().currentFrameSync()),
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

	// TODO: adjust neighbourCellFinderInputInfos according to expected sortResult
	auto sortResult = _radixSort.stage(
		buf,
		30,
		initInfos, pingInfos, pongInfos
	);

	size_t pingBufferSegment = sortResult == RadixSort::Result::InPingBuffer ? 0 : 1;
	size_t pongBufferSegment = sortResult == RadixSort::Result::InPingBuffer ? 1 : 0;

	_neighbourCellFinder(buf, _particleData.size(), _tempBuffer.segment(pingBufferSegment), _gridDataBuffer.fullBufferInfo());

	static constexpr size_t numSteps = 5;
	for (size_t step = 0; step < numSteps; ++step) {
		bool isLast = step == (numSteps - 1);
		_context.bindPipeline(
			buf, _calcLambdaPipeline,
			{
				{_tempBuffer.segment(pingBufferSegment), _gridDataBuffer.fullBufferInfo()},
				{_neighbourCellFinder.gridBoundaryBuffer().fullBufferInfo()},
				{_lambdaBuffer.fullBufferInfo()}
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
							 isLast ? _particleKeys.segment(_context.renderer().nextFrameSync())
							 		: _tempBuffer.segment(pongBufferSegment)
						 }
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
			{_particleKeys.segment(_context.renderer().nextFrameSync())},
			{_particleData.segment(_context.renderer().nextFrameSync())},
		}
	);
	buf.dispatch(((getNumParticles() + blockSize - 1) / blockSize), 1, 1);

	buf.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader, {}, {
		vk::MemoryBarrier{
			.srcAccessMask = vk::AccessFlagBits::eShaderWrite,
			.dstAccessMask = vk::AccessFlagBits::eShaderRead
		}
	}, {}, {});



}

}