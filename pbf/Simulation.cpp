#include "Simulation.h"
#include "VulkanContext.h"
#include "Renderer.h"
#include "Scene.h"
#include <pbf/descriptors/DescriptorSet.h>
#include <imgui.h>
#include <random>

namespace pbf {

namespace {

[[nodiscard]] auto initializeSystem(ParticleData* data, size_t numParticles, std::vector<DistanceConstraintSolver::Constraint>* distanceConstraints = nullptr)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(-0.25f, 0.25f);

	size_t bladeLength = 6;
	size_t bladeHeight = 15;
	size_t numBlades = 5;
	size_t numParticlesRotator = bladeHeight * (4 + numBlades * bladeLength * 2);
	float y_shift = -50.0f;

	assert(numParticlesRotator < numParticles);
	size_t numParticlesFluid = numParticles - numParticlesRotator;

	// Fluid Particles
    {
	    size_t edgeLength = std::ceil(std::cbrt(numParticlesFluid));
		std::set<std::pair<uint32_t, uint32_t>> borderParticlePairs;

		auto isBorder = [&](int32_t x, int32_t y, int32_t z)
		{
			return ((x < 3) || (x >= edgeLength - 3)) ||
									((y < 3) || (y >= edgeLength - 3)) ||
									((z < 3) || (z >= edgeLength - 3));
		};

		auto calcId = [&](int32_t x, int32_t y, int32_t z)
		{
			return x * edgeLength * edgeLength + y * edgeLength + z;
		};
		auto isInSystem = [&](int32_t x, int32_t y, int32_t z)
		{
			return x >= 0 && y >= 0 && z >= 0 && x < edgeLength && y < edgeLength && z < edgeLength && calcId(x,y,z) < numParticlesFluid;
		};

	    [&](){
	        for (int32_t x = 0; x < edgeLength; ++x)
	        {
	            for (int32_t y = 0; y < edgeLength; ++y)
	            {
	                for (int32_t z = 0; z < edgeLength; ++z)
	                {
                		int32_t id = calcId(x, y, z);
	                    if (id >= numParticlesFluid)
	                        return;
	                    data[id].position = glm::vec3(x - 32, 20-63 + y, z - 32);
	                    //data[id].position += glm::vec3(dist(gen), dist(gen), dist(gen));
	                    data[id].position *= 0.8f;
                		data[id].aux = (id % 256 == 0) ? -1u : 0;
	                    data[id].velocity = glm::vec3(0,0,0);
	                    data[id].type = 0;
                		if (isBorder(x, y, z) && distanceConstraints)
                		{
                			/*distanceConstraints->push_back(DistanceConstraintSolver::Constraint(
								id, 0, 0, 1.0f,
								glm::vec4(data[id].position, 0.0f)
							));*/
                			for (int32_t dx = -1; dx <= 1; ++dx)
                				for (int32_t dy = -1; dy <= 1; ++dy)
                					for (int32_t dz = -1; dz <= 1; ++dz)
                					{
                						if (isBorder(x + dx, y + dy, z + dz) && isInSystem(x + dx, y + dy, z + dz))
                						{
                							int32_t id_j = calcId(x + dx, y + dy, z + dz);
                							if (id < id_j)
	                							borderParticlePairs.insert(std::make_pair(id, id_j));
                						}
                					}
                		}
	                }
	            }
	        }
	    }();
		if (false && distanceConstraints)
		{
			/*const float restDist = 1.0f;
			for (size_t i = 1; i < 1000; ++i)
			{
				distanceConstraints->push_back(DistanceConstraintSolver::Constraint(
					i, i+1, restDist, 0.001f,
					glm::vec4()
				));
				distanceConstraints->push_back(DistanceConstraintSolver::Constraint(
					i-1, i+1, restDist * 2.0f, 0.0001f,
					glm::vec4()
				));
				distanceConstraints->push_back(DistanceConstraintSolver::Constraint(
					i, i+2, restDist * 2.0f, 0.0001f,
					glm::vec4()
				));
			}*/
			for (auto [i, j]: borderParticlePairs)
			{
				assert(glm::distance(data[i].position / 0.8f, data[j].position / 0.8f) < 3.0f);
				distanceConstraints->push_back(DistanceConstraintSolver::Constraint(
					i, j, glm::distance(data[i].position, data[j].position),
					0.001f,1.0f, 0.01f, glm::vec2()
				));
			}
		}
    }

	// Rotator Particles
/*	size_t bladeLength = 4;
	size_t bladeHeight = 4;
	size_t numParticlesRotator = bladeHeight * (4 + 4 * bladeLength * 2);
*/
	glm::mat4 transform = glm::rotate(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, y_shift, 0.0f)), glm::radians(81.0f), glm::vec3(1, 0, 0));

	auto set = [data, transform](size_t id, glm::vec3 pos) {
		data[id].type = 1;
		auto p = transform * glm::vec4(pos, 1.0f);
		data[id].position = glm::vec3(p) / p.w;
	};
    std::move_only_function<void(std::vector<PositionConstraintSolver::Constraint>& _contraints, float _angle)const> positionConstraintFiller = [](...){};
    {
    	auto id = numParticlesFluid;


    	for (size_t heightBase = 0; heightBase < bladeHeight; ++heightBase)
    	{
    		float height = float(heightBase) - float(bladeHeight) / 2.0f;
    		size_t const startId = id;
    		auto centerQuadPositions = std::array{glm::vec3(-0.5, height, -0.5),
			glm::vec3(0.5, height, -0.5),
			glm::vec3(-0.5, height, 0.5),
			glm::vec3(0.5, height, 0.5)};
    		for (auto p: centerQuadPositions)
    			set(id++, p);
    		positionConstraintFiller = [transform, startId = startId, centerQuadPositions = std::move(centerQuadPositions), previousFiller = std::move(positionConstraintFiller)](std::vector<PositionConstraintSolver::Constraint>& _constraints, float _angle)
    		{
    			previousFiller(_constraints, _angle);
    			auto rot = glm::rotate(glm::mat4(1), _angle, glm::vec3(0, 1, 0));
    			for (size_t i = 0; i < centerQuadPositions.size(); ++i)
    			{
    				glm::vec4 p = transform * glm::vec4(glm::vec3(rot * glm::vec4(centerQuadPositions[i], 1.0)), 1.0);
    				_constraints.push_back(PositionConstraintSolver::Constraint(
						glm::vec3(p) / p.w,
						startId + i, 0.0, 0.01f, 400.0f, 0.01f
					));

    			}
    		};

    		std::vector<glm::vec3> dirs;
    		float angleShift = (float(height) / float(bladeHeight)) * glm::two_pi<float>() / float(5);
    		for (size_t i = 0; i < numBlades; ++i)
	    		dirs.emplace_back(glm::rotate(glm::mat4(1), angleShift + float(i) / float(numBlades) * glm::two_pi<float>(), glm::vec3(0, 1, 0)) * glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
    		for (auto dir:dirs)
    		{
    			glm::vec3 normal= glm::vec3(0, 1, 0);
    			glm::vec3 tangent = glm::cross(dir, normal);
    			for (size_t i = 0; i < bladeLength; ++i)
    			{
    				set(id++, glm::vec3(0, height, 0) + float(1 + i) * dir + 0.5f*tangent);
    				set(id++, glm::vec3(0, height, 0) + float(1 + i) * dir - 0.5f*tangent);
    			}
    		}
    	}
    	for (size_t i = 0; i < numParticlesRotator - 1; ++i)
    	{
    		for (size_t j = i+1; j < numParticlesRotator; ++j)
    		{
    			if (distanceConstraints)
    			{
    				float dist = glm::distance(data[numParticlesFluid + i].position, data[numParticlesFluid + j].position);
    				if (dist < 6.0)
    				distanceConstraints->push_back(DistanceConstraintSolver::Constraint(
						numParticlesFluid + i, numParticlesFluid + j, dist,
						0.001f, 1.0f, 0.01f, glm::vec2()
					));
    			}
    		}

    	}
    	assert(id == numParticles);
    }

	return positionConstraintFiller;

    //std::shuffle(data, data + numParticles, gen);
}

constexpr auto radixSortDescriptorSetLayoutDescriptors() {
	return std::vector<descriptors::DescriptorSetLayout>{descriptors::DescriptorSetLayout{
		.createFlags = {},
		.bindings = {{
						 .binding = 0,
						 .descriptorType = vk::DescriptorType::eStorageBuffer,
						 .descriptorCount = 1
					 },{
						 .binding = 1,
						 .descriptorType = vk::DescriptorType::eUniformBuffer,
						 .descriptorCount = 1
					 },{
						 .binding = 2,
						 .descriptorType = vk::DescriptorType::eStorageBuffer,
						 .descriptorCount = 1
					 }},
		PBF_DESC_DEBUG_NAME("global sort and hash Particle Key Layout")
	}};
}
}

Simulation::Simulation(InitContext &initContext, Renderer& renderer, GUI& gui, size_t numParticles):
UIControlled(gui),
_context(initContext.context),
renderer(renderer),
_particleData{
	initContext.context,
	numParticles,
	2,
	vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer,
	pbf::MemoryType::STATIC
},
_particleKeys(initContext.context, _particleData.size(), _particleData.segments(), vk::BufferUsageFlagBits::eStorageBuffer|vk::BufferUsageFlagBits::eTransferDst, MemoryType::STATIC),
_gridDataBuffer(_context, 1, vk::BufferUsageFlagBits::eUniformBuffer|vk::BufferUsageFlagBits::eTransferDst, MemoryType::STATIC),
_lambdaBuffer(initContext.context, _particleData.size(), vk::BufferUsageFlagBits::eStorageBuffer|vk::BufferUsageFlagBits::eTransferDst, MemoryType::STATIC),
_vorticityBuffer(initContext.context, _particleData.size(), vk::BufferUsageFlagBits::eStorageBuffer, MemoryType::STATIC),
_radixSort(_context, blockSize, getNumParticles() / blockSize, radixSortDescriptorSetLayoutDescriptors(), "shaders/particlesort"),
_tempBuffer(_context, _particleData.size(), 2, vk::BufferUsageFlagBits::eStorageBuffer|vk::BufferUsageFlagBits::eTransferSrc, MemoryType::STATIC)
{

	{
		auto& context = initContext.context;
		RingBuffer<ParticleData>& particleData = _particleData;

		auto& initBuffer = initContext.createInitData<Buffer<ParticleData>>(
				context, numParticles, vk::BufferUsageFlagBits::eTransferSrc, pbf::MemoryType::TRANSIENT
		);

		ParticleData* data = initBuffer.data();
		std::vector<DistanceConstraintSolver::Constraint> distanceConstraints;
		std::vector<PositionConstraintSolver::Constraint> positionConstraints;
		positionConstraintFiller = initializeSystem(data, numParticles, &distanceConstraints);
		positionConstraintFiller(positionConstraints, 0.0f);
		auto& initCmdBuf = *initContext.initCommandBuffer;
		_distanceConstraintSolver = std::make_unique<pbf::DistanceConstraintSolver>(initContext, gui, distanceConstraints);
		_positionConstraintSolver = std::make_unique<pbf::PositionConstraintSolver>(initContext, gui, positionConstraints);
		initBuffer.flush();

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
	}

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
		std::construct_at<GridData>(gridDataInitBuffer.data(), glm::ivec3(-128, -128, -128), glm::ivec3(127, 127, 127), h);
		_neighbourCellFinder = std::make_unique<NeighbourCellFinder>(_context, gridDataInitBuffer.data()->numCells(), getNumParticles());
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

	buildPipelines();
	initKeys(initContext.context, *initContext.initCommandBuffer);
}

void Simulation::reset(vk::CommandBuffer &buf) {
    auto& initBuffer = renderer.createFrameData<Buffer<ParticleData>>(
            _context, _particleData.size(), vk::BufferUsageFlagBits::eTransferSrc, pbf::MemoryType::TRANSIENT
    );
    ParticleData* data = initBuffer.data();

    positionConstraintFiller = initializeSystem(data, _particleData.size());
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
	currentRotatorAngle = 0.0f;
}

std::string Simulation::uiCategory() const
{
	return "Simulation";
}

void Simulation::ui()
{
	ImGui::Checkbox("Run Distance Constraint Solver", &_runDistanceConstraintSolver);
	ImGui::SliderFloat("maximum timestep", &maxTimestep, 0.0001f, 0.01f, "%.5f");
	ImGui::SliderFloat("key power", &keyPower, 0.1f, 20.0f, "%.3f");
	ImGui::SliderFloat("rotator speed", &rotatorSpeed, -20.0f, 20.0f, "%.3f");
	bool rebuildPipelines = false;
	rebuildPipelines |= ImGui::SliderFloat("h", &h, 0.25f, 4.0f, "%.3f");
	rebuildPipelines |= ImGui::SliderFloat("rho_0_type_0", &rho_0_type_0, 0.1f, 10.0f, "%.1f");
	rebuildPipelines |= ImGui::SliderFloat("rho_0_type_1", &rho_0_type_1, 0.1f, 10.0f, "%.1f");
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
		Specialization<float>{.constantID = 2, .value = 1.0f / rho_0_type_0},
		Specialization<float>{.constantID = 3, .value = epsilon},
		Specialization<float>{.constantID = 4, .value = xsph_viscosity_c},
		Specialization<float>{.constantID = 5, .value = tensile_instability_k},
		Specialization<float>{.constantID = 6, .value = vorticity_epsilon},
		Specialization<float>{.constantID = 7, .value = tensile_instability_scale},
		Specialization<float>{.constantID = 8, .value = 1.0f / rho_0_type_1},
	};
}


void Simulation::initKeys(VulkanContext& context, vk::CommandBuffer buf)
{
	Cache& cache = context.cache();
	{
		for (size_t i = 0; i < _particleData.segments(); ++i) {
			_context.bindPipeline(
				buf, _keyInitPipeline,
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
	if (timestep > maxTimestep)
	{
		timestep = std::min(timestep, maxTimestep);
	}
	if (_resetKeys)
	{
		initKeys(_context, buf);
		_resetKeys = false;
	}

	{
		auto& copyBuffer = renderer.createFrameData<Buffer<PositionConstraintSolver::Constraint>>(
				_context, _positionConstraintSolver->numConstraints(), vk::BufferUsageFlagBits::eTransferSrc, pbf::MemoryType::TRANSIENT
		);
		auto* data = copyBuffer.data();
		std::vector<PositionConstraintSolver::Constraint> constraints;
		positionConstraintFiller(constraints, currentRotatorAngle);
		for (auto&& [id, c]: constraints | std::ranges::views::enumerate)
			data[id] = c;
		_positionConstraintSolver->setConstraintsFromBuffer(buf, copyBuffer.buffer());
		currentRotatorAngle -= timestep * rotatorSpeed;
	}

	_context.bindPipeline(buf, _unconstrainedSystemUpdatePipeline, {
		{_particleKeys.segment(ringBufferIndex)},
		{_particleData.segment(ringBufferIndex)}
	});
	static constexpr float Gabs = 9.81f;
	UnconstrainedPositionUpdatePushConstants pushConstants{
		.externalAccell = glm::vec3(0.0f, -2.0f*Gabs, 0.0f),
		.lastTimestep = _lastTimestep,
		.timestep = timestep
	};
	_lastTimestep = timestep;
	pushConstants.externalAccell += keyPower * Gabs * glm::vec3(_context.window().getKey(GLFW_KEY_LEFT) ? 1.0f : 0.0f, 0, _context.window().getKey(GLFW_KEY_UP) ? 1.0f : 0.0f);
	pushConstants.externalAccell += keyPower * Gabs * glm::vec3(_context.window().getKey(GLFW_KEY_RIGHT) ? -1.0f : 0.0f, 0, _context.window().getKey(GLFW_KEY_DOWN) ? -1.0f : 0.0f);

	buf.pushConstants(*(_unconstrainedSystemUpdatePipeline->pipelineLayout), vk::ShaderStageFlagBits::eAll, 0, sizeof(pushConstants), &pushConstants);

	buf.dispatch(((getNumParticles() + blockSize - 1) / blockSize), 1, 1);

	buf.fillBuffer(_lambdaBuffer.buffer(), 0, _lambdaBuffer.deviceSize(), 0);

	buf.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader|vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eComputeShader, {}, {
		vk::MemoryBarrier{
			.srcAccessMask = vk::AccessFlagBits::eShaderWrite|vk::AccessFlagBits::eTransferWrite,
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

	/* up to date info in:
	* 		_particleKeys.segment(ringBufferIndex)
	*		_particleData.segment(ringBufferIndex)
	*		initInfos
	*/

	auto distanceConstraintRunner = _runDistanceConstraintSolver ? std::make_optional(_distanceConstraintSolver->startSolverLoop(buf)) : std::nullopt;
	auto positionConstraintRunner = _positionConstraintSolver->startSolverLoop(buf);
	static constexpr size_t numSteps = 3;
	for (size_t step = 0; step < numSteps; ++step)
	{

        // TODO: adjust neighbourCellFinderInputInfos according to expected sortResult -> probably done
        auto sortResult = _radixSort.stage(
                buf,
                30,
                initInfos, pingInfos, pongInfos
        );

        size_t pingBufferSegment = sortResult == RadixSort::Result::InPingBuffer ? 0 : 1;
        size_t pongBufferSegment = sortResult == RadixSort::Result::InPingBuffer ? 1 : 0;

		/* up to date info in:
		* 		_particleKeys.segment(ringBufferIndex)
		*		_particleData.segment(ringBufferIndex)
		*		initInfos
		*	sorted up to date info in:
		*		_tempBuffer.segment(pingBufferSegment)
		*/

        (*_neighbourCellFinder)(buf, _particleData.size(), _tempBuffer.segment(pingBufferSegment), _gridDataBuffer.fullBufferInfo());



        bool isLast = step == (numSteps - 1);
		_context.bindPipeline(
			buf, _calcLambdaPipeline,
			{
				{_tempBuffer.segment(pingBufferSegment), _gridDataBuffer.fullBufferInfo()},
				{_neighbourCellFinder->gridBoundaryBuffer().fullBufferInfo()},
				{_lambdaBuffer.fullBufferInfo()},
				{_particleData.segment(nextRingBufferIndex())}
			}
		);
		buf.dispatch(((getNumParticles() + blockSize - 1) / blockSize), 1, 1);
		buf.fillBuffer(_lambdaBuffer.buffer(), 0, _lambdaBuffer.deviceSize(), 0);

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
							 _neighbourCellFinder->gridBoundaryBuffer().fullBufferInfo()
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


		/* sorted up to date info in:
		*		_tempBuffer.segment(pongBufferSegment)
		*/

		_context.bindPipeline(buf, _copyParticleKeysToDatePipeline, {
			{_tempBuffer.segment(pongBufferSegment)},
			{_particleData.segment(nextRingBufferIndex())}
		});
		buf.dispatch(((getNumParticles() + blockSize - 1) / blockSize), 1, 1);

		buf.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader, {}, {
			vk::MemoryBarrier{
				.srcAccessMask = vk::AccessFlagBits::eShaderWrite,
				.dstAccessMask = vk::AccessFlagBits::eShaderRead
			}
		}, {}, {});

		/* up to date info in:
 		 *		_tempBuffer.segment(pongBufferSegment)
		 *		_particleData.segment(nextRingBufferIndex())
		 */

		// Assumed to perform its update in place in _particleData.segment(nextRingBufferIndex())
		if (distanceConstraintRunner)
			(*distanceConstraintRunner)(buf, timestep, _particleData.segment(nextRingBufferIndex()), _particleData.segment(ringBufferIndex));
		positionConstraintRunner(buf, timestep, _particleData.segment(nextRingBufferIndex()), _particleData.segment(ringBufferIndex));

		// copy positions from particle data to particle keys (or adjust radix sort input)
		//		_particleData.segment(nextRingBufferIndex()) -> _particleKeys.segment(ringBufferIndex)

		_context.bindPipeline(buf, _keyInitPipeline, {
				{{_particleData.segment(nextRingBufferIndex()), _particleKeys.segment(ringBufferIndex)}}
		});
		buf.dispatch(((getNumParticles() + blockSize - 1) / blockSize), 1, 1);

		buf.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader, {}, {
			vk::MemoryBarrier{
				.srcAccessMask = vk::AccessFlagBits::eShaderWrite,
				.dstAccessMask = vk::AccessFlagBits::eShaderRead
			}
		}, {}, {});
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
	buf.pushConstants(*(_particleDataUpdatePipeline->pipelineLayout), vk::ShaderStageFlagBits::eAll, 0, sizeof(float), &timestep);
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
			{_neighbourCellFinder->gridBoundaryBuffer().fullBufferInfo()},
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
			{_neighbourCellFinder->gridBoundaryBuffer().fullBufferInfo()},
			{_vorticityBuffer.fullBufferInfo()},
			{_particleData.segment(ringBufferIndex)},
			{_particleData.segment(nextRingBufferIndex())}
		}
	);
	buf.pushConstants(*(_updateVelPipeline->pipelineLayout), vk::ShaderStageFlagBits::eAll, 0, sizeof(float), &timestep);
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

	auto& cache = _context.cache();
	{
		_keyInitPipeline = cache.fetch(
			descriptors::ComputePipeline{
				.flags = {},
				.shaderStage = descriptors::ShaderStage {
					.module = cache.fetch(
						descriptors::ShaderModule{
							.source = descriptors::ShaderModule::File{"shaders/simulation/keyinit.comp.spv"},
							PBF_DESC_DEBUG_NAME("Simulation: key init shader module")
						}),
					.specialization = {
						Specialization<uint32_t>{.constantID = 0, .value = blockSize}
					}
				},
				PBF_DESC_DEBUG_NAME("Simulation: key init shader pipeline")
			}
		);
	}
	{
		_particleDataUpdatePipeline = cache.fetch(
			descriptors::ComputePipeline{
				.flags = {},
				.shaderStage = descriptors::ShaderStage {
					.module = cache.fetch(
					descriptors::ShaderModule{
						.source = descriptors::ShaderModule::File{"shaders/simulation/particledataupdate.comp.spv"},
						PBF_DESC_DEBUG_NAME("Simulation: particle data update shader module")
					}),
					.specialization = specializationInfo
				},
				PBF_DESC_DEBUG_NAME("Simulation: particle data update shader pipeline")
			}
		);
	}
	{
		_copyParticleKeysToDatePipeline = cache.fetch(
			descriptors::ComputePipeline{
				.flags = {},
				.shaderStage = descriptors::ShaderStage {
					.module = cache.fetch(
					descriptors::ShaderModule{
						.source = descriptors::ShaderModule::File{"shaders/simulation/copyparticlekeystodata.comp.spv"},
						PBF_DESC_DEBUG_NAME("Simulation: copy particle keys to particle data shader")
					}),
					.specialization = specializationInfo // TODO: maybe remove; all unused in shader
				},
				PBF_DESC_DEBUG_NAME("Simulation: copy particle keys to particle data pipeline")
			}
		);
	}
	{
		_unconstrainedSystemUpdatePipeline = cache.fetch(
			descriptors::ComputePipeline{
				.flags = {},
				.shaderStage = descriptors::ShaderStage {
					.module = cache.fetch(
					descriptors::ShaderModule{
						.source = descriptors::ShaderModule::File{"shaders/simulation/unconstrainedupdate.comp.spv"},
						PBF_DESC_DEBUG_NAME("Simulation: unconstrained system update shader (Update positions based on velocity and external forces without considering constraint violations.)")
					}),
					.specialization = specializationInfo
				},
				PBF_DESC_DEBUG_NAME("Simulation: unconstrained system update pipeline (Update positions based on velocity and external forces without considering constraint violations.)")
			}
		);
	}

	{
		_calcLambdaPipeline = cache.fetch(
			descriptors::ComputePipeline{
				.flags = {},
				.shaderStage = descriptors::ShaderStage {
					.module = cache.fetch(
					descriptors::ShaderModule{
						.source = descriptors::ShaderModule::File{"shaders/simulation/calclambda.comp.spv"},
						PBF_DESC_DEBUG_NAME("Simulation: Calc Lambda Shader")
					}),
					.specialization = specializationInfo
				},
				PBF_DESC_DEBUG_NAME("Simulation: calc lambda pipeline")
			}
		);

		_calcVorticityPipeline = cache.fetch(
			descriptors::ComputePipeline{
				.flags = {},
				.shaderStage = descriptors::ShaderStage {
					.module = cache.fetch(
					descriptors::ShaderModule{
						.source = descriptors::ShaderModule::File{"shaders/simulation/calcvorticity.comp.spv"},
						PBF_DESC_DEBUG_NAME("Simulation: Calc Vorticity Shader")
					}),
					.specialization = specializationInfo
				},
				PBF_DESC_DEBUG_NAME("Simulation: calc vorticity pipeline")
			}
		);

		_updateVelPipeline = cache.fetch(
			descriptors::ComputePipeline{
				.flags = {},
				.shaderStage = descriptors::ShaderStage {
					.module = cache.fetch(
					descriptors::ShaderModule{
						.source = descriptors::ShaderModule::File{"shaders/simulation/updatevel.comp.spv"},
						PBF_DESC_DEBUG_NAME("Simulation: Update Vel Shader")
					}),
					.specialization = specializationInfo
				},
				PBF_DESC_DEBUG_NAME("Simulation: update vel pipeline")
			}
		);

		_updatePosPipeline = cache.fetch(
			descriptors::ComputePipeline{
				.flags = {},
				.shaderStage = descriptors::ShaderStage {
					.module = cache.fetch(
					descriptors::ShaderModule{
						.source = descriptors::ShaderModule::File{"shaders/simulation/updatepos.comp.spv"},
						PBF_DESC_DEBUG_NAME("Simulation: Update Pos Shader")
					}),
					.specialization = specializationInfo
				},
				PBF_DESC_DEBUG_NAME("Simulation: update pos pipeline")
			}
		);
	}
}

}
