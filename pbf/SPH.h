#pragma once

#include "common.h"
#include "Buffer.h"
#include "Cache.h"
#include <pbf/descriptors/ComputePipeline.h>

namespace pbf {

class SPH
{
public:
	SPH(InitContext& initContext);
	SPH(const SPH&) = delete;
	SPH& operator=(const SPH&) = delete;
	~SPH() = default;
private:
	CacheReference<descriptors::ComputePipeline> _calcLambdaPipeline;

};

}
