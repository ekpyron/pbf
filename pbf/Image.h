//
// Created by daniel on 04.07.22.
//

#ifndef PBF_IMAGE_H
#define PBF_IMAGE_H

#include "common.h"
#include "Context.h"
#include "MemoryManager.h"

namespace pbf {

class Image
{
public:
	Image(Context& context, vk::Format format, vk::ImageUsageFlags usageFlags, vk::Extent3D const& extents);
	Image(Image&& _image) = default;
	~Image() = default;
	vk::Image image() const {
		return *_image;
	}
private:
	Context& _context;
	vk::UniqueImage _image;
	DeviceMemory _imageMemory;
};

}

#endif //PBF_IMAGE_H
