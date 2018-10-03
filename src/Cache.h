/**
 *
 *
 * @file Factory.h
 * @brief 
 * @author clonker
 * @date 10/3/18
 */
#pragma once

#include <cstddef>
#include <map>
#include <algorithm>

namespace pbf {

namespace detail {
template<typename T>
struct CachedObject {
    T object;
    std::size_t lastUsageFrameNumber;
};
}

template<typename Result>
class Cache {
public:

    explicit Cache(std::size_t lifetime) : _lifetime(lifetime) {}

    Result &fetch(typename Result::Descriptor descriptor) {
        auto it = std::find(std::begin(_map), std::end(_map), descriptor);
        if (it == std::end(_map)) {
            return (_map[descriptor] = {
                    .object = Result(descriptor),
                    .lastUsageFrameNumber = _currentFrame
            }).object;
        }
        return it->object;
    }

    void frame() {
        ++_currentFrame;
        auto it = std::begin(_map);
        while (it != std::end(_map)) {
            if (it->lastUsageFrameNumber + _lifetime < _currentFrame) {
                it = _map.erase(it);
            } else {
                ++it;
            }
        }
    }

private:
    std::size_t _currentFrame = 0;
    std::size_t _lifetime;

    std::map<typename Result::Descriptor, detail::CachedObject<Result>> _map;
};
}
