//
// Created by daniel on 04.07.22.
//

#ifndef PBF_IMAGE_H
#define PBF_IMAGE_H

#include "common.h"
#include "VulkanContext.h"
#include "MemoryManager.h"

namespace pbf {

class Image
{
public:
	Image(VulkanContext& context, vk::ImageCreateFlags createFlags, vk::Format format, vk::ImageUsageFlags usageFlags, vk::Extent3D const& extents);
	Image(VulkanContext& context, vk::Format format, vk::ImageUsageFlags usageFlags, vk::Extent3D const& extents):
	Image(context, {}, format, usageFlags, extents) {}
	Image(Image&& _image) = default;
	~Image() = default;
	vk::Image image() const {
		return *_image;
	}
private:
	VulkanContext& _context;
	vk::UniqueImage _image;
	DeviceMemory _imageMemory;
};

}

#endif //PBF_IMAGE_H
