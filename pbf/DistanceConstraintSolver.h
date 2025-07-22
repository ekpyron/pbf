#pragma once
#include "common.h"
#include "Cache.h"
#include "Buffer.h"

#include <pbf/descriptors/ComputePipeline.h>

namespace pbf
{

class DistanceConstraintSolver {
public:
    DistanceConstraintSolver();
    ~DistanceConstraintSolver() = default;
    DistanceConstraintSolver(const DistanceConstraintSolver&) = delete;
    DistanceConstraintSolver& operator=(const DistanceConstraintSolver&) = delete;

    void run(vk::CommandBuffer buf, vk::DescriptorBufferInfo const& _particleDataInOut);

private:
    struct Constraint
    {
        // dot(particlepos[index_i] - particlepos[index_j], particlepos[index_i] - particlepos[index_j]) - distance² = 0.0
        uint32_t index_i = 0;
        uint32_t index_j = 0;
        float distance = 1.0;
        float alpha = 1.0; // stiffness
    };
    Buffer<Constraint> constraints;
    // potentially: std::vector<Buffer<Constraint>> for graph-color batched constraint sets.
    Buffer<float> lambdas;

    CacheReference<Pipeline> calcLambda;
    CacheReference<Pipeline> updatePosition;
    uint blockSize = 256;

};

}