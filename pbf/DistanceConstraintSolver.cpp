#include "DistanceConstraintSolver.h"

#include "imgui.h"
#include "Scene.h"

namespace pbf
{

DistanceConstraintSolver::DistanceConstraintSolver(InitContext& _initContext, GUI& gui, std::vector<Constraint> const& _constraints):
    UIControlled(gui),
    _context(_initContext.context),
    constraints(_initContext.context, _constraints.size(), vk::BufferUsageFlagBits::eTransferDst|vk::BufferUsageFlagBits::eStorageBuffer, MemoryType::STATIC),
    lambdas(_initContext.context, _constraints.size(), vk::BufferUsageFlagBits::eTransferDst|vk::BufferUsageFlagBits::eStorageBuffer, MemoryType::STATIC)
{
    auto& constraintInitBuffer = _initContext.createInitData<Buffer<Constraint>>(
            _context, _constraints.size(), vk::BufferUsageFlagBits::eTransferSrc, pbf::MemoryType::TRANSIENT
    );

    std::ranges::copy(_constraints, constraintInitBuffer.data());
    _initContext.initCommandBuffer->copyBuffer(
        constraintInitBuffer.buffer(),
        constraints.buffer(),
        {vk::BufferCopy{
            0, 0, constraints.deviceSize()
        }
        }
    );

    _initContext.initCommandBuffer->pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eComputeShader, {}, {
        vk::MemoryBarrier{
            .srcAccessMask = vk::AccessFlagBits::eTransferWrite,
            .dstAccessMask = vk::AccessFlagBits::eShaderRead
        }
    }, {}, {});

    buildPipelines();
}


std::string DistanceConstraintSolver::uiCategory() const
{
    return "Distance Constraint Solver";
}

void DistanceConstraintSolver::ui()
{
    ImGui::SliderFloat("force constant factor", &forceConstantFactor, 0.001f, 20.0f, "%.3f");
    ImGui::SliderFloat("alpha factor", &alphaFactor, 0.001f, 100.0f, "%.3f");
    ImGui::SliderFloat("beta factor", &betaFactor, 0.001f, 100.0f, "%.3f");

}

void DistanceConstraintSolver::_startSolverLoop(vk::CommandBuffer buf)
{
    buf.fillBuffer(lambdas.buffer(), 0, lambdas.deviceSize(), 0);
    buf.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eComputeShader, {}, {
    vk::MemoryBarrier{
        .srcAccessMask = vk::AccessFlagBits::eTransferWrite,
        .dstAccessMask = vk::AccessFlagBits::eShaderRead
    }
}, {}, {});
}


void DistanceConstraintSolver::_run(vk::CommandBuffer buf, float _timestep, vk::DescriptorBufferInfo const& _particleDataInOut, vk::DescriptorBufferInfo const& _previousParticleData)
{
    // For reference:
    // https://matthias-research.github.io/pages/publications/XPBD.pdf
    // https://raw.githubusercontent.com/InteractiveComputerGraphics/SPlisHSPlasH/master/doc/images/teaser.gif
    // https://matthias-research.github.io/pages/publications/PBDTutorial2017-slides-1.pdf
    // https://matthias-research.github.io/pages/publications/PBDTutorial2017-slides-2.pdf
    // https://matthias-research.github.io/pages/publications/flex.pdf
    // https://matthias-research.github.io/pages/tenMinutePhysics/16-GPUSimulation.pdf
    //
    // https://matthias-research.github.io/pages/tenMinutePhysics/09-xpbd.pdf
    // Old non xpbd: https://matthias-research.github.io/pages/publications/posBasedDyn.pdf

    /*
     * Random notes:
     *
     * For density constraints, our solver loop is *also* a loop over all constraints really.
     * But there is exactly one constraint per particle (that the estimated density around it is rest density).
     * So iterating over all *particles*, in that case, is the same as iterating over all constraints.
     *
     * For distance constraints, we calculate a fixed set of particle pairs to be constrained up front and
     * store them in @m constraints and iterate over those.
     * The solver can be build following https://matthias-research.github.io/pages/publications/XPBD.pdf
     *
     * Since multiple distance constraints act on the same particles, position updates need to be atomic.
     *
     * Ideally, the constraints are graph-colored according to affected particles, s.t. constraint solving can
     * be batched by color to avoid locking in the atomic operations. (I.e. @m constraints should potentially be generated
     * as already split into color-coded batches manually on initialization.)
     *
     */

    constexpr size_t numSteps = 1;

    struct PushConstants
    {
        float timestep = 0.01f;
        float forceConstantFactor = 1.0f;
        float alphaFactor = 1.0f;
        float betaFactor = 1.0f;
    };
    PushConstants pushConstants{_timestep, forceConstantFactor, alphaFactor, betaFactor};

    for (size_t distancestep = 0; distancestep < numSteps; ++distancestep)
    {
        // TODO: barriers?

        _context.bindPipeline(buf, calcLambda, {
            {vk::DescriptorBufferInfo{
                constraints.buffer(), 0, constraints.deviceSize()
            }}, // set 0
            {
                vk::DescriptorBufferInfo{
                    lambdas.buffer(), 0, lambdas.deviceSize()
                }
            }, // set 1
            // in, out, orig
            {_particleDataInOut, _previousParticleData}
        });
        buf.pushConstants(*calcLambda->pipelineLayout, vk::ShaderStageFlagBits::eAll, 0, sizeof(pushConstants), &pushConstants);

        buf.dispatch((constraints.size()  + blockSize - 1) / blockSize, 1, 1);

        buf.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader, {}, {
            vk::MemoryBarrier{
                .srcAccessMask = vk::AccessFlagBits::eShaderWrite,
                .dstAccessMask = vk::AccessFlagBits::eShaderRead
            }
        }, {}, {});
    }

#if 0
    // TODO: barriers
    buf.bindPipeline(vk::PipelineBindPoint::eCompute, distanceConstraints.updatePosition);
    // TODO bind buffers
    buf.dispatch((distanceConstraints.constraints.size()  + distanceConstraints.blockSize - 1) / distanceConstraints.blockSize, 1, 1);
    // TODO: barriers
#endif
}

void DistanceConstraintSolver::buildPipelines()
{
    auto& cache = _context.cache();
    calcLambda = cache.fetch(
        descriptors::ComputePipeline{
            .flags = {},
            .shaderStage = descriptors::ShaderStage {
                .module = cache.fetch(
                descriptors::ShaderModule{
                    .source = descriptors::ShaderModule::File{"shaders/simulation/constraints/distance/calclambda.comp.spv"},
                    PBF_DESC_DEBUG_NAME("Simulation: Constraints: Distance: Calc Lambda Shader")
                }),
                .specialization = {
                    Specialization<uint32_t>{.constantID = 0, .value = blockSize},
                    Specialization<uint32_t>{.constantID = 9, .value = static_cast<uint32_t>(constraints.size())},
                }
            },
            PBF_DESC_DEBUG_NAME("Simulation: Constraints: Distance: calc lambda pipeline")
        }
    );
}

}
