//
// Created by Tonz on 30.07.2025.
//

#pragma once
#include <cstdint>
#include <mutex>

class ResourceManagerBase {
public:
    static ResourceManagerBase* getInstance(){

        if (instance_ == nullptr) {
            instance_ = new ResourceManagerBase();
        }
        return instance_;
    }

    uint32_t assignGlobalId() {
        std::lock_guard<std::mutex> lock(counterMutex_);
        return ++globalIdCounter_;
    }

    virtual ~ResourceManagerBase() = default;

protected:
    ResourceManagerBase() = default;

private:
    inline static ResourceManagerBase* instance_{nullptr};
    uint32_t globalIdCounter_{0};
    std::mutex counterMutex_;
};
