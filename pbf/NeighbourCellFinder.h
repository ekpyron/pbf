#pragma once

#include <pbf/common.h>
#include <pbf/descriptors/ComputePipeline.h>
#include <pbf/Buffer.h>

namespace pbf {

class NeighbourCellFinder
{
public:
	NeighbourCellFinder(ContextInterface& context, size_t numGridCells, size_t maxID, MemoryType gridBoundaryBufferMemoryType = MemoryType::STATIC);

    struct GridData {
    	GridData(glm::ivec3 const& _min, glm::ivec3 const& _max, float h):
    	max(_max.x, _max.y, _max.z, 0),
    	min(_min.x, _min.y, _min.z, 0),
    	hashweights(1, (_max.x - _min.x + 1), (_max.x - _min.x + 1) * (_max.y - _min.y + 1)),
    	cellSize(h)
    	{
    	}
    	GridData() = delete;
        glm::ivec4 max;
        glm::ivec4 min;
        glm::ivec3 hashweights;
    	float cellSize;
    	glm::vec3 maxGridCoords() const
    	{
    		return cellSize * glm::vec3(max.x, max.y, max.z);
    	}
    	glm::vec3 minGridCoords() const
    	{
    		return cellSize * glm::vec3(min.x, min.y, min.z);
    	}
    	[[nodiscard]] inline size_t numCells() const {
            glm::ivec3 gridExtents = glm::ivec3(max) - glm::ivec3(min);
            return (gridExtents.x + 1) * (gridExtents.y + 1) * (gridExtents.z + 1);
        }
    };

	static constexpr auto inputDescriptorSetLayout() {
		return descriptors::DescriptorSetLayout{
			.createFlags = {},
			.bindings = {
				{
					.binding = 0,
					.descriptorType = vk::DescriptorType::eStorageBuffer,
					.descriptorCount = 1
				},
				{
					.binding = 1,
					.descriptorType = vk::DescriptorType::eUniformBuffer,
					.descriptorCount = 1
				}
			},
			PBF_DESC_DEBUG_NAME("Neighbour Cell Finder Input Descriptor Set Layout")
		};
	}

	void operator()(vk::CommandBuffer buf, uint32_t numParticles, vk::DescriptorBufferInfo const& input, vk::DescriptorBufferInfo const& gridData) const;

	struct alignas(glm::uvec4) GridBoundaries {
		uint startIndex = 0;
		uint endIndex = 0;
        std::strong_ordering operator<=>(GridBoundaries const& _rhs) const = default;
	};

	const Buffer<GridBoundaries>& gridBoundaryBuffer() const {
		return _gridBoundaryBuffer;
	}

private:
	static constexpr uint32_t blockSize = 256;

	ContextInterface& _context;

	Buffer<GridBoundaries> _gridBoundaryBuffer;

	CacheReference<descriptors::ComputePipeline> _findCellsPipeline;
};

}
