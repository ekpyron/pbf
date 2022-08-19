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
#include <pbf/common.h>
#include <pbf/descriptors/Order.h>
#include <pbf/VulkanObjectType.h>

namespace pbf {

template<typename T>
class CacheReference;

class TypedCacheBase {
public:
    virtual ~TypedCacheBase() = default;
    virtual void frame() = 0;
};

class Cache {
public:

    explicit Cache(ContextInterface *context, std::size_t lifetime) : _context(context), _lifetime(lifetime) {}

    template<typename T>
    CacheReference<descriptors::remove_cv_ref_t<T>> fetch(T &&descriptor);

    void frame();

    std::size_t currentFrame() const {
        return _currentFrame;
    }

    std::size_t lifetime() const {
        return _lifetime;
    }

	ContextInterface* context() {
        return _context;
    }

#ifndef NDEBUG
    void setGenericObjectName(vk::ObjectType type, uint64_t obj, const std::string& name) const;
#endif

private:
    std::size_t _currentFrame = 0;
    ContextInterface *_context;
    std::size_t _lifetime;

    std::unordered_map<std::type_index, std::unique_ptr<TypedCacheBase>> _map;
};

template<typename T>
concept DescriptorType = requires(T t) {t.descriptor();};

template<typename T>
class CachedObject {
public:
	template<typename U>
    CachedObject(Cache *cache, U &&objdesc) : _cache(cache), _objdesc(std::forward<U>(objdesc)) {}

#ifndef NDEBUG
    ~CachedObject() {
        if (_obj)
            spdlog::get("console")->debug("[Cache] Destroy (cache cleanup) [{}]", objectDebugName());
    }
#endif

    CachedObject(const CachedObject &) = delete;

    CachedObject &operator=(const CachedObject &) = delete;

    auto &get() const {
        keepDepsAlive();
        if (!_obj) {
#ifndef NDEBUG
            _objectIncarnation++;
            spdlog::get("console")->debug("[Cache] Create [{}]", objectDebugName());
#endif
            _obj = _objdesc.realize(_cache->context());
#ifndef NDEBUG
            // if constexpr (KnownVulkanObject<std::decay_t<decltype(*_obj)>>) {
            if constexpr (KnownVulkanObject<std::decay_t<decltype(*_obj)>>) {
                if (!_objdesc.debugName.empty()) {
                    _cache->setGenericObjectName(
                            VulkanObjectType<std::decay_t<decltype(*_obj)>>,
                            reinterpret_cast<uint64_t>(static_cast<void*>(*_obj)),
                            objectDebugName());
                }
            }

#endif
        }
        _lastUsedFrameNumber = _cache->currentFrame();
        return *_obj;
    }

    void frame() const {
        if (_lastUsedFrameNumber + _cache->lifetime() < _cache->currentFrame()) {
#ifndef NDEBUG
            if (_obj)
                spdlog::get("console")->debug("[Cache] Destroy (inactive) [{}]", objectDebugName());
#endif
            _obj.reset();
        }
    }

    const T &descriptor() const {
        return _objdesc;
    }

private:
    const T _objdesc;
    Cache *_cache = nullptr;
    mutable std::size_t _lastUsedFrameNumber = 0;
    mutable decltype(_objdesc.realize(_cache->context())) _obj;
#ifndef NDEBUG
    mutable std::uint64_t _objectIncarnation = 0;
    std::string objectDebugName() const {
        if (!_objdesc.debugName.empty()) {
            return fmt::format("{} <{}>", _objdesc.debugName, _objectIncarnation);;
        } else {
            return fmt::format("{:#x} <{}>", reinterpret_cast<uintptr_t>(&_objdesc), _objectIncarnation);
        }
    }
#endif

    template<typename Q, typename = void>
    struct keepDependenciesAlive {
        void operator()(const T&) const {}
    };
    template<typename Q>
    struct keepDependenciesAlive<Q, std::void_t<typename Q::template Depends<Q>>> {
        void operator()(const T& obj) const {
            crampl::ForEachMemberInList<typename T::template Depends<T>>::call(obj, [](const auto &ref) {
                ref.keepAlive();
            });
        }
    };


    void keepDepsAlive() const {
		if constexpr (DescriptorType<T>) {
			keepDependenciesAlive<T>()(_obj->descriptor());
		}
    }
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

    void keepAlive() const {
        **this;
    }

    explicit operator bool() const {
        return _obj;
    }

    const T &descriptor() const {
        return _obj->descriptor();
    }

    bool operator<(const CacheReference &rhs) const {
        return _obj < rhs._obj;
    }

	static constexpr bool AllowNativeComparison = true;
private:

    const CachedObject<T> *_obj = nullptr;
};

template<typename T>
class TypedCache : public TypedCacheBase {
public:
    TypedCache(Cache *cache) : _cache(cache) {}

	template<typename U>
    CacheReference<T> fetch(U &&descriptor) {
        auto it = _set.find(descriptor);
        if (it == _set.end())
            it = _set.emplace(_cache, std::forward<U>(descriptor)).first;
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
            return descriptors::Order<T>()(lhs.descriptor(), rhs.descriptor());
        }

        bool operator()(const CachedObject<T> &lhs, const T &descriptor) const {
            return descriptors::Order<T>()(lhs.descriptor(), descriptor);
        }

        bool operator()(const T &descriptor, const CachedObject<T> &rhs) const {
            return descriptors::Order<T>()(descriptor, rhs.descriptor());
        }
    };

    std::set<CachedObject<T>, CachedObjectCompare> _set;
    Cache *_cache = nullptr;
};

template<typename T>
inline CacheReference<descriptors::remove_cv_ref_t<T>> Cache::fetch(T&& descriptor) {
	using U = descriptors::remove_cv_ref_t<T>;
    auto typedCache = _map.find(std::type_index(typeid(U)));
    if (typedCache == _map.end()) {
        typedCache = _map.emplace(std::type_index(typeid(U)),
                                  std::make_unique<TypedCache<U>>(this)).first;
    }
    return static_cast<TypedCache<U> *>(typedCache->second.get())->fetch(std::forward<T>(descriptor));
}

inline void Cache::frame() {
    ++_currentFrame;
    for (auto const &typedCache : _map) {
        typedCache.second->frame();
    }
}

}
