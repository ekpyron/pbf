#include "DistanceConstraintSolver.h"

namespace pbf
{

DistanceConstraintSolver::DistanceConstraintSolver() = default;

void DistanceConstraintSolver::run(vk::CommandBuffer buf, vk::DescriptorBufferInfo const& _particleDataInOut)
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
#if 0
    // TODO: barriers
    buf.bindPipeline(vk::PipelineBindPoint::eCompute, distanceConstraints.calcLambda);
    // TODO bind buffers
    buf.dispatch((distanceConstraints.constraints.size()  + distanceConstraints.blockSize - 1) / distanceConstraints.blockSize, 1, 1);
    // TODO: barriers
    buf.bindPipeline(vk::PipelineBindPoint::eCompute, distanceConstraints.updatePosition);
    // TODO bind buffers
    buf.dispatch((distanceConstraints.constraints.size()  + distanceConstraints.blockSize - 1) / distanceConstraints.blockSize, 1, 1);
    // TODO: barriers
#endif
}


}