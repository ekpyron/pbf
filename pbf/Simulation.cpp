#include "Simulation.h"
#include "Context.h"
#include "Renderer.h"
#include "Scene.h"
#include <pbf/descriptors/DescriptorSet.h>

namespace pbf {

namespace {
constexpr auto radixSortDescriptorSetLayout() {
	return descriptors::DescriptorSetLayout{
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
			},
			{
				.binding = 2,
				.descriptorType = vk::DescriptorType::eUniformBuffer,
				.descriptorCount = 1,
				.stageFlags = vk::ShaderStageFlagBits::eCompute
			},
			{
				.binding = 3,
				.descriptorType = vk::DescriptorType::eStorageBuffer,
				.descriptorCount = 1,
				.stageFlags = vk::ShaderStageFlagBits::eCompute
			}
		},
		PBF_DESC_DEBUG_NAME("Simulation particle sort descriptor set layout")
	};
}
}

Simulation::Simulation(InitContext &initContext, RingBuffer<ParticleData>& particleData):
_context(initContext.context),
_particleData(particleData),
_particleKeys(initContext.context, particleData.size(), particleData.segments(), vk::BufferUsageFlagBits::eStorageBuffer|vk::BufferUsageFlagBits::eTransferDst, MemoryType::STATIC),
_gridDataBuffer(_context, 1, vk::BufferUsageFlagBits::eUniformBuffer|vk::BufferUsageFlagBits::eTransferDst, MemoryType::STATIC),
_lambdaBuffer(initContext.context, particleData.size(), vk::BufferUsageFlagBits::eStorageBuffer, MemoryType::STATIC),
_radixSort(_context, blockSize, getNumParticles() / blockSize, radixSortDescriptorSetLayout(), "shaders/particlesort"),
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
		std::vector pingPongLayouts(2 + particleData.segments(), *_context.cache().fetch(radixSortDescriptorSetLayout()));
		initDescriptorSets = _context.device().allocateDescriptorSets(vk::DescriptorSetAllocateInfo{
			.descriptorPool = _context.descriptorPool(),
			.descriptorSetCount = size32(pingPongLayouts),
			.pSetLayouts = pingPongLayouts.data()
		});
		pingDescriptorSet = initDescriptorSets[initDescriptorSets.size() - 2];
		pongDescriptorSet = initDescriptorSets[initDescriptorSets.size() - 1];
		initDescriptorSets.resize(initDescriptorSets.size() - 2);
	}
	{
		std::vector layouts(2, *_context.cache().fetch(NeighbourCellFinder::inputDescriptorSetLayout()));
		auto sets = _context.device().allocateDescriptorSets(vk::DescriptorSetAllocateInfo{
			.descriptorPool = _context.descriptorPool(),
			.descriptorSetCount = size32(layouts),
			.pSetLayouts = layouts.data()
		});
		neighbourCellFinderInputDescriptorSets[0] = sets[0];
		neighbourCellFinderInputDescriptorSets[1] = sets[1];
	}

	vk::DescriptorBufferInfo gridDataBufferInfo{
		.buffer = _gridDataBuffer.buffer(),
		.offset = 0u,
		.range = sizeof(GridData)
	};
	for (size_t i = 0; i < _particleKeys.segments(); ++i)
	{
		auto initDescriptorSet = initDescriptorSets[i];
		auto initDescriptorInfos = {
			_particleKeys.segment(i),
			_tempBuffer.segment(1)
		};
		_context.device().updateDescriptorSets({vk::WriteDescriptorSet{
			.dstSet = initDescriptorSet,
			.dstBinding = 0,
			.dstArrayElement = 0,
			.descriptorCount = static_cast<uint32_t>(initDescriptorInfos.size()),
			.descriptorType = vk::DescriptorType::eStorageBuffer,
			.pImageInfo = nullptr,
			.pBufferInfo = &*initDescriptorInfos.begin(),
			.pTexelBufferView = nullptr
		}, vk::WriteDescriptorSet{
			.dstSet = initDescriptorSet,
			.dstBinding = 2,
			.dstArrayElement = 0,
			.descriptorCount = 1,
			.descriptorType = vk::DescriptorType::eUniformBuffer,
			.pImageInfo = nullptr,
			.pBufferInfo = &gridDataBufferInfo,
			.pTexelBufferView = nullptr
		}}, {});
	}

	{
		auto pingDescriptorInfos = {
			_tempBuffer.segment(0),
			_tempBuffer.segment(1)
		};
		auto pongDescriptorInfos = {
			_tempBuffer.segment(1),
			_tempBuffer.segment(0)
		};
		_context.device().updateDescriptorSets({vk::WriteDescriptorSet{
			.dstSet = pingDescriptorSet,
			.dstBinding = 0,
			.dstArrayElement = 0,
			.descriptorCount = static_cast<uint32_t>(pingDescriptorInfos.size()),
			.descriptorType = vk::DescriptorType::eStorageBuffer,
			.pImageInfo = nullptr,
			.pBufferInfo = &*pingDescriptorInfos.begin(),
			.pTexelBufferView = nullptr
		}, vk::WriteDescriptorSet{
			.dstSet = pingDescriptorSet,
			.dstBinding = 2,
			.dstArrayElement = 0,
			.descriptorCount = 1,
			.descriptorType = vk::DescriptorType::eUniformBuffer,
			.pImageInfo = nullptr,
			.pBufferInfo = &gridDataBufferInfo,
			.pTexelBufferView = nullptr
		}, vk::WriteDescriptorSet{
			.dstSet = pongDescriptorSet,
			.dstBinding = 0,
			.dstArrayElement = 0,
			.descriptorCount = static_cast<uint32_t>(pongDescriptorInfos.size()),
			.descriptorType = vk::DescriptorType::eStorageBuffer,
			.pImageInfo = nullptr,
			.pBufferInfo = &*pongDescriptorInfos.begin(),
			.pTexelBufferView = nullptr
		}, vk::WriteDescriptorSet{
			.dstSet = pongDescriptorSet,
			.dstBinding = 2,
			.dstArrayElement = 0,
			.descriptorCount = 1,
			.descriptorType = vk::DescriptorType::eUniformBuffer,
			.pImageInfo = nullptr,
			.pBufferInfo = &gridDataBufferInfo,
			.pTexelBufferView = nullptr
		}}, {});
	}
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

	{
		std::vector layouts(2, *singleStorageBufferDescriptorSetLayout);
		auto sets = _context.device().allocateDescriptorSets(vk::DescriptorSetAllocateInfo{
			.descriptorPool = _context.descriptorPool(),
			.descriptorSetCount = size32(layouts),
			.pSetLayouts = layouts.data()
		});
		_tempBufferDescriptorSets[0] = sets[0];
		_tempBufferDescriptorSets[1] = sets[1];
	}

	for(size_t i = 0; i < 2; ++i)
	{
		auto neighbourCellFinderInputInfos = {
			_tempBuffer.segment(i) // TODO may also be zero! Depending on sort bits.
		};
		_context.device().updateDescriptorSets({vk::WriteDescriptorSet{
			.dstSet = neighbourCellFinderInputDescriptorSets[i],
			.dstBinding = 0,
			.dstArrayElement = 0,
			.descriptorCount = 1,
			.descriptorType = vk::DescriptorType::eStorageBuffer,
			.pImageInfo = nullptr,
			.pBufferInfo = &*neighbourCellFinderInputInfos.begin(),
			.pTexelBufferView = nullptr
		},vk::WriteDescriptorSet{
			.dstSet = _tempBufferDescriptorSets[i],
			.dstBinding = 0,
			.dstArrayElement = 0,
			.descriptorCount = 1,
			.descriptorType = vk::DescriptorType::eStorageBuffer,
			.pImageInfo = nullptr,
			.pBufferInfo = &*neighbourCellFinderInputInfos.begin(),
			.pTexelBufferView = nullptr
		},vk::WriteDescriptorSet{
			.dstSet = neighbourCellFinderInputDescriptorSets[i],
			.dstBinding = 1,
			.dstArrayElement = 0,
			.descriptorCount = 1,
			.descriptorType = vk::DescriptorType::eUniformBuffer,
			.pImageInfo = nullptr,
			.pBufferInfo = &gridDataBufferInfo,
			.pTexelBufferView = nullptr
		}}, {});
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

	{
		lambdaDescriptorSet = _context.device().allocateDescriptorSets(vk::DescriptorSetAllocateInfo{
			.descriptorPool = _context.descriptorPool(),
			.descriptorSetCount = uint32_t(1),
			.pSetLayouts = &*singleStorageBufferDescriptorSetLayout
		}).front();

		vk::DescriptorBufferInfo bufferInfo{
			.buffer = _lambdaBuffer.buffer(),
			.offset = 0,
			.range = _lambdaBuffer.deviceSize()
		};
		_context.device().updateDescriptorSets({vk::WriteDescriptorSet{
			.dstSet = lambdaDescriptorSet,
			.dstBinding = 0,
			.dstArrayElement = 0,
			.descriptorCount = 1,
			.descriptorType = vk::DescriptorType::eStorageBuffer,
			.pImageInfo = nullptr,
			.pBufferInfo = &bufferInfo,
			.pTexelBufferView = nullptr
		}}, {});
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

vk::DescriptorSet makeDescriptorSet(
	Cache& cache,
	CacheReference<descriptors::DescriptorSetLayout> setLayout,
	std::vector<descriptors::DescriptorSetBinding> const& bindings
)
{
	return *cache.fetch(
		descriptors::DescriptorSet{
			.setLayout = setLayout,
			.bindings = bindings
		}
	);
}

template<typename PipelineType>
void bindPipeline(
	ContextInterface& context,
	vk::CommandBuffer buf,
	CacheReference<PipelineType> pipeline,
	std::vector<std::vector<descriptors::DescriptorSetBinding>> const& bindings
)
{
	Cache& cache = context.cache();
	buf.bindPipeline(PipelineType::bindPoint, *pipeline);
	CacheReference<descriptors::PipelineLayout> pipelineLayout = pipeline.descriptor().pipelineLayout;
	auto const& setLayouts = pipelineLayout.descriptor().setLayouts;
	std::vector<vk::DescriptorSet> descriptorSets;
	for (size_t i = 0; i < setLayouts.size(); ++i)
	{
		descriptorSets.emplace_back(*cache.fetch(
			descriptors::DescriptorSet{
				.setLayout = setLayouts[i],
				.bindings = bindings[i]
			}
		));
	}
	buf.bindDescriptorSets(
		PipelineType::bindPoint,
		*pipelineLayout,
		0,
		descriptorSets,
		{}
	);
}

// currentFrameSync (readonly) -> nextFrameSync (writeonly)
void Simulation::run(vk::CommandBuffer buf, float timestep)
{
	if (_resetKeys)
	{
		initKeys(_context, buf);
		_resetKeys = false;
	}
	bindPipeline(_context, buf, _unconstrainedSystemUpdatePipeline, {
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


	// TODO: adjust neighbourCellFinderInputInfos according to expected sortResult
	auto sortResult = _radixSort.stage(buf, 30, initDescriptorSets[_context.renderer().currentFrameSync()], pingDescriptorSet, pongDescriptorSet);

	size_t pingBufferSegment = sortResult == RadixSort::Result::InPingBuffer ? 0 : 1;
	size_t pongBufferSegment = sortResult == RadixSort::Result::InPingBuffer ? 1 : 0;

	_neighbourCellFinder(buf, _particleData.size(), neighbourCellFinderInputDescriptorSets[pingBufferSegment]);

	static constexpr size_t numSteps = 5;
	for (size_t step = 0; step < numSteps; ++step) {
		bool isLast = step == (numSteps - 1);
		buf.bindPipeline(vk::PipelineBindPoint::eCompute, *_calcLambdaPipeline);
		buf.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *(_calcLambdaPipeline.descriptor().pipelineLayout), 0, {
			neighbourCellFinderInputDescriptorSets[pingBufferSegment], _neighbourCellFinder.gridData(), lambdaDescriptorSet
		}, {});
		buf.dispatch(((getNumParticles() + blockSize - 1) / blockSize), 1, 1);

		buf.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader, {}, {
			vk::MemoryBarrier{
				.srcAccessMask = vk::AccessFlagBits::eShaderWrite,
				.dstAccessMask = vk::AccessFlagBits::eShaderRead
			}
		}, {}, {});


		bindPipeline(_context, buf, _updatePosPipeline,
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

	bindPipeline(
		_context,
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