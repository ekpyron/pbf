#pragma once

#include <pbf/common.h>
#include <pbf/Image.h>
#include <pbf/Buffer.h>

#include "App.h"

namespace pbf {

struct ParticleData;

class Selection
{
public:
	Selection(InitContext& initContext, Renderer& renderer, GlobalAppData& globalData);

	std::optional<uint32_t> operator()(RingBuffer<ParticleData>& particleData, uint32_t x, uint32_t y);

private:
	Renderer& m_renderer;
	GlobalAppData& m_globalData;
	VulkanContext& _context;
	Image _depthBuffer;
	Image _selectionImage;
	Buffer<std::uint16_t> _indexBuffer;
	vk::UniqueImageView _depthBufferView;
	vk::UniqueImageView _selectionImageView;
	Buffer<std::uint32_t> _selectionBuffer;
	vk::UniqueFramebuffer _framebuffer;
	CacheReference<descriptors::RenderPass> _renderPass;
	CacheReference<descriptors::GraphicsPipeline> _graphicsPipeline;
};

}
