#pragma once

#include <vector>
#include <memory>
#include "Renderer.h"

namespace pbf
{

template<typename T>
class FrameSyncData {
public:
    FrameSyncData(Renderer& _renderer): renderer(_renderer)
    {
    }
    template<typename... Args>
    FrameSyncData(Renderer& _renderer, std::in_place_t, Args&&... args): renderer(_renderer)
    {
        create(std::forward<Args>(args)...);
    }

    ~FrameSyncData() = default;

    template<typename... Args>
    void create(Args&&... args)
    {
        for(auto i = 0; i < renderer.framePrerenderCount(); ++i)
            data.emplace_back(std::make_unique<T>(std::forward<Args>(args)...));
    }

    T& getCurrent()
    {
        return *data.at(renderer.currentFrameSync());
    }
    T& get(uint32_t _i)
    {
        return *data.at(_i);
    }
    template<typename Callable>
    void forEach(Callable _callable)
    {
        for (auto& elem: data)
            _callable(*elem);
    }
private:
    Renderer& renderer;
    std::vector<std::unique_ptr<T>> data;
};

}
