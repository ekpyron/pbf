#pragma once
#include "common.h"
#include "Cache.h"
#include "Buffer.h"

#include <pbf/descriptors/ComputePipeline.h>
#include "VulkanContext.h"
#include "contrib/Catch2/src/catch2/internal/catch_clara.hpp"

#include "contrib/Catch2/src/catch2/internal/catch_context.hpp"

namespace pbf
{
    struct ParticleData;

    struct DistanceConstraintSolverConfig
    {
        struct Constraint
        {
            // dot(particlepos[index_i] - particlepos[index_j], particlepos[index_i] - particlepos[index_j]) - distance² = 0.0
            uint32_t index_i = 0;
            uint32_t index_j = 0;
            float distance = 1.0;
            float alpha = 1.0; // stiffness
            float forceConstant = 1.0;
            float beta = 0.01; // damping
            glm::vec2 aux;
        };
        static constexpr auto uiCategory() { return "Distance Constraint Solver"; }
        static constexpr auto shader() { return "shaders/simulation/constraints/distance/calclambda.comp.spv"; }
    };

    struct PositionConstraintSolverConfig
    {
        struct Constraint
        {
            glm::vec3 position{};
            uint32_t index_i = 0;
            float distance = 0.0;
            float alpha = 1.0; // stiffness
            float forceConstant = 1.0;
            float beta = 0.01; // damping
        };
        static constexpr auto uiCategory() { return "Position Constraint Solver"; }
        static constexpr auto shader() { return "shaders/simulation/constraints/position/calclambda.comp.spv"; }
    };

    template<typename ConstraintSolverConfig>
    class GenericConstraintSolver: public UIControlled {
public:
    using Constraint = ConstraintSolverConfig::Constraint;
    GenericConstraintSolver(InitContext& _initContext, GUI& gui, std::vector<Constraint> const& _constraints);
    ~GenericConstraintSolver() = default;
    GenericConstraintSolver(const GenericConstraintSolver&) = delete;
    GenericConstraintSolver& operator=(const GenericConstraintSolver&) = delete;
    auto startSolverLoop(vk::CommandBuffer buf) -> auto
    {
        _startSolverLoop(buf);
        return [&]<typename... Args>(Args&&... args) { return _run(std::forward<Args>(args)...); };
    }

    void setConstraintsFromBuffer(vk::CommandBuffer buf, vk::Buffer buffer);
    size_t numConstraints() const
    {
        return constraints.size();
    }

protected:
    std::string uiCategory() const override;

private:
    void ui() override;
    void _startSolverLoop(vk::CommandBuffer buf); // To be called once outside the outer simulation loop.
    void _run(vk::CommandBuffer buf, float _timestep, vk::DescriptorBufferInfo const& _particleDataInOut, vk::DescriptorBufferInfo const& _previousParticleData);
    VulkanContext& _context;
    Buffer<Constraint> constraints;
    // potentially: std::vector<Buffer<Constraint>> for graph-color batched constraint sets.
    Buffer<float> lambdas;

    CacheReference<descriptors::ComputePipeline> calcLambda;
    CacheReference<descriptors::ComputePipeline> updatePosition;
    uint blockSize = 256;
    float forceConstantFactor = 1.0f;
    float alphaFactor = 1.0f;
    float betaFactor = 1.0f;

    void buildPipelines();

};

using DistanceConstraintSolver = GenericConstraintSolver<DistanceConstraintSolverConfig>;
using PositionConstraintSolver = GenericConstraintSolver<PositionConstraintSolverConfig>;

}
