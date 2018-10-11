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
    CacheReference<T> fetch(T &&descriptor);

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
    CachedObject(Cache *cache, T &&objdesc) : _cache(cache), _objdesc(std::move(objdesc)) {}

    CachedObject(const CachedObject &) = delete;

    CachedObject &operator=(const CachedObject &) = delete;

    auto &get() const {
        if (!_obj)
            _obj = _objdesc.realize(_cache->context());
        _lastUsedFrameNumber = _cache->currentFrame();
        return *_obj;
    }

    void frame() const {
        if (_lastUsedFrameNumber + _cache->lifetime() < _cache->currentFrame()) {
            _obj.reset();
        }
    }

    const T &descriptor() const {
        return _objdesc;
    }

private:
    T _objdesc;
    Cache *_cache = nullptr;
    mutable std::size_t _lastUsedFrameNumber = 0;
    mutable decltype(_objdesc.realize(_cache->context())) _obj;
};

template<typename T>
class CacheReference {
public:
    CacheReference() : _obj(nullptr) {}

    CacheReference(const CachedObject<T> *obj) : _obj(obj) {}

    const auto &operator*() const {
        if (!_obj) throw std::runtime_error("Empty cache reference dereferenced");
        return _obj->get();
    }

    const auto *operator->() const {
        return &**this;
    }

    void keepAlive() {
        **this;
    }

    void keepAlive() const {
        **this;
    }

    operator bool() const {
        return _obj;
    }

    bool operator<(const CacheReference &rhs) const {
        return _obj < rhs._obj;
    }

private:
    const CachedObject<T> *_obj = nullptr;
};

template<typename T>
class TypedCache : public TypedCacheBase {
public:
    TypedCache(Cache *cache) : _cache(cache) {}

    CacheReference<T> fetch(T &&descriptor) {
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

        bool operator()(const CachedObject<T> &lhs, const T &descriptor) const {
            return lhs.descriptor() < descriptor;
        }

        bool operator()(const T &descriptor, const CachedObject<T> &rhs) const {
            return descriptor < rhs.descriptor();
        }
    };

    std::set<CachedObject<T>, CachedObjectCompare> _set;
    Cache *_cache = nullptr;
};

template<typename T>
inline CacheReference<T> Cache::fetch(T&& descriptor) {
    auto typedCache = _map.find(std::type_index(typeid(T)));
    if (typedCache == _map.end()) {
        typedCache = _map.emplace(std::type_index(typeid(T)),
                                  std::make_unique<TypedCache<T>>(this)).first;
    }
    return static_cast<TypedCache<T> *>(typedCache->second.get())->fetch(std::forward<T>(descriptor));
}

inline void Cache::frame() {
    ++_currentFrame;
    for (auto const &typedCache : _map) {
        typedCache.second->frame();
    }
}

}
