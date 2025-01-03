#include <pbf/Cache.h>
#include <pbf/VulkanContext.h>

using namespace pbf;

#ifndef NDEBUG
void Cache::setGenericObjectName(vk::ObjectType type, uint64_t obj, const std::string& name) const {
    _context.setGenericObjectName(type, obj, name);
}
#endif
