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
#include <set>

namespace pbf {

class Context;

template<typename T>
class CacheReference;

class TypedCacheBase {
public:
    virtual ~TypedCacheBase() = default;

    virtual void frame() = 0;
};

class Cache {
public:

    explicit Cache(Context *context, std::size_t lifetime) : _context(context), _lifetime(lifetime) {}

    template<typename T>
    CacheReference<typename T::Result> fetch(T &&descriptor);

    void frame();

    std::size_t currentFrame() const {
        return _currentFrame;
    }

    std::size_t lifetime() const {
        return _lifetime;
    }

    Context* context() {
        return _context;
    }

private:
    std::size_t _currentFrame = 0;
    Context *_context;
    std::size_t _lifetime;

    std::unordered_map<std::type_index, std::unique_ptr<TypedCacheBase>> _map;
};

template<typename T>
class CachedObject {
public:
    CachedObject(Cache *cache, typename T::Descriptor &&objdesc) : _cache(cache), _objdesc(std::move(objdesc)) {}

    CachedObject(const CachedObject &) = delete;

    CachedObject &operator=(const CachedObject &) = delete;

    T &get() const {
        if (!_obj)
            _obj = std::make_unique<T>(_cache->context(), _objdesc);
        _lastUsedFrameNumber = _cache->currentFrame();
        return *_obj;
    }

    void frame() const {
        if (_lastUsedFrameNumber + _cache->lifetime() < _cache->currentFrame()) {
            _obj.reset();
        }
    }

    const typename T::Descriptor &descriptor() const {
        return _objdesc;
    }

private:
    typename T::Descriptor _objdesc;
    Cache *_cache = nullptr;
    mutable std::size_t _lastUsedFrameNumber = 0;
    mutable std::unique_ptr<T> _obj;
};

template<typename T>
class CacheReference {
public:
    CacheReference() : _obj(nullptr) {}

    CacheReference(const CachedObject<T> *obj) : _obj(obj) {}

    T &operator*() {
        if (!_obj) throw std::runtime_error("Empty cache reference dereferenced");
        return _obj->get();
    }

    T *operator->() {
        return &**this;
    }

    void keepAlive() {
        **this;
    }

    operator bool() const {
        return _obj;
    }

private:
    const CachedObject<T> *_obj = nullptr;
};

template<typename Descriptor>
class TypedCache : public TypedCacheBase {
public:
    using T = typename Descriptor::Result;

    TypedCache(Cache *cache) : _cache(cache) {}
/*
    template<typename D>
    CacheReference<T> fetch(D&& descriptor) {
        auto it = _set.find(descriptor);
        if (it == _set.end())
            it = _set.emplace(_cache, std::forward<D>(descriptor)).first;
        return {&*it};
    }
*/

    CacheReference<T> fetch(Descriptor &&descriptor) {
        auto it = _set.find(descriptor);
        if (it == _set.end())
            it = _set.emplace(_cache, std::move(descriptor)).first;
        return {&*it};
    }

    void frame() override {
        for (auto &obj : _set) {
            obj.frame();
        }
    }

private:
    struct CachedObjectCompare {
        using is_transparent = void;
        bool operator()(const CachedObject<T> &lhs, const CachedObject<T> &rhs) const {
            return lhs.descriptor() < rhs.descriptor();
        }

        bool operator()(const CachedObject<T> &lhs, const Descriptor &descriptor) const {
            return lhs.descriptor() < descriptor;
        }

        bool operator()(const Descriptor &descriptor, const CachedObject<T> &rhs) const {
            return descriptor < rhs.descriptor();
        }
    };

    std::set<CachedObject<T>, CachedObjectCompare> _set;
    Cache *_cache = nullptr;
};

template<typename Descriptor>
inline CacheReference<typename Descriptor::Result> Cache::fetch(Descriptor&& descriptor) {
    auto typedCache = _map.find(std::type_index(typeid(Descriptor)));
    if (typedCache == _map.end()) {
        typedCache = _map.emplace(std::type_index(typeid(Descriptor)),
                                  std::make_unique<TypedCache<Descriptor>>(this)).first;
    }
    return static_cast<TypedCache<Descriptor> *>(typedCache->second.get())->fetch(std::forward<Descriptor>(descriptor));
}

inline void Cache::frame() {
    ++_currentFrame;
    for (auto const &typedCache : _map) {
        typedCache.second->frame();
    }
}

}
