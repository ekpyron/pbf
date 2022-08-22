//
// Created by daniel on 04.07.22.
//

#include "Image.h"

namespace pbf {

Image::Image(Context& context, vk::Format format, vk::ImageUsageFlags usageFlags, vk::Extent3D const& _extents): _context(context)
{
	uint32_t queueFamily = context.graphicsQueueFamily();
	_image = context.device().createImageUnique(vk::ImageCreateInfo{
		.flags = {},
		.imageType = vk::ImageType::e2D,
		.format = format,
		.extent = _extents,
		.mipLevels = 1,
		.arrayLayers = 1,
		.samples = vk::SampleCountFlagBits::e1,
		.tiling = vk::ImageTiling::eOptimal,
		.usage = usageFlags,
		.sharingMode = vk::SharingMode::eExclusive,
		.queueFamilyIndexCount = 1,
		.pQueueFamilyIndices = &queueFamily,
		.initialLayout = vk::ImageLayout::eUndefined
	});

	_imageMemory = context.memoryManager().allocateImageMemory(MemoryType::STATIC, *_image);

}

}