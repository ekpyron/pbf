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
#include <typeindex>
#include <memory>
#include <unordered_map>

namespace pbf {

template<typename T>
class CacheReference;

class TypedCacheBase {
public:
    virtual ~TypedCacheBase() {
    }
    virtual void frame() = 0;
};

class Cache {
public:

    explicit Cache(std::size_t lifetime) : _lifetime(lifetime) {}

    template<typename T>
    CacheReference<typename T::Result> fetch(const T& descriptor);
    void frame();
    std::size_t currentFrame() const {
        return _currentFrame;
    }
    std::size_t lifetime() const {
        return _lifetime;
    }
private:
    std::size_t _currentFrame = 0;
    std::size_t _lifetime;

    std::unordered_map<std::type_index, std::unique_ptr<TypedCacheBase>> _map;
};

template<typename T>
class CachedObject {
public:
    CachedObject(Cache *cache, const typename T::Descriptor &objdesc) : _cache(cache), _objdesc(objdesc) {}
    CachedObject(const CachedObject&) = delete;
    CachedObject& operator=(const CachedObject&) = delete;
    T &get() {
        if (!_obj)
            _obj = std::make_unique<T>(_objdesc);
        _lastUsedFrameNumber = _cache->currentFrame();
        return *_obj;
    }

    void frame(void) {
        if (_lastUsedFrameNumber + _cache->lifetime() < _cache->currentFrame()) {
            _obj.reset();
        }
    }

private:
    typename T::Descriptor _objdesc;
    Cache *_cache = nullptr;
    std::size_t _lastUsedFrameNumber = 0;
    std::unique_ptr<T> _obj;
};

template<typename T>
class CacheReference {
public:
    CacheReference() : _obj(nullptr) {}
    CacheReference(CachedObject<T>* obj) : _obj(obj) {
    }
    T &operator*() {
        if (!_obj) throw std::runtime_error("Empty cache reference dereferenced");
        return _obj->get();
    }
    operator bool() const {
        return _obj;
    }
private:
    CachedObject<T> *_obj = nullptr;
};

template<typename Descriptor>
class TypedCache : public TypedCacheBase {
public:
    using T = typename Descriptor::Result;
    TypedCache(Cache *cache) : _cache(cache) {}
    CacheReference<T> fetch(const Descriptor& descriptor) {
        auto it = _map.find(descriptor);
        if (it == _map.end())
            it = _map.emplace(std::piecewise_construct, std::forward_as_tuple(descriptor), std::forward_as_tuple(_cache, descriptor)).first;
        return { &it->second };
    }

    void frame() override {
        for (auto& obj : _map) {
            obj.second.frame();
        }
    }

private:
    std::map<Descriptor, CachedObject<T>> _map;
    Cache *_cache = nullptr;
};

template<typename Descriptor>
inline CacheReference<typename Descriptor::Result> Cache::fetch(const Descriptor& descriptor) {
    auto typedCache = _map.find(std::type_index(typeid(Descriptor)));
    if (typedCache == _map.end()) {
        typedCache = _map.emplace(std::type_index(typeid(Descriptor)), std::make_unique<TypedCache<Descriptor>>(this)).first;
    }
    return static_cast<TypedCache<Descriptor>*>(typedCache->second.get())->fetch(descriptor);
}

inline void Cache::frame() {
    ++_currentFrame;
    for (auto const& typedCache : _map) {
        typedCache.second->frame();
    }
}


/*

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
        it->lastUsageFrameNumber = _currentFrame;
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
};*/
}
