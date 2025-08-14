//
// Created by Tonz on 28.07.2025.
//

#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <iostream>
#include <type_traits>

#include "managedResource.h"
#include "resourceManagerBase.h"
class ManagedResource;

template <typename T>
concept ManagedResourceConcept = std::is_base_of_v<ManagedResource,T>;

template <ManagedResourceConcept T>
class ResourceManager : public ResourceManagerBase {
public:

    static ResourceManager<T>* getInstance(){
        if (instance_ == nullptr)
            instance_ = new ResourceManager<T>();
        return instance_;
    }

    virtual std::shared_ptr<T>  getResource(std::string_view resourceName) {
        std::cerr << "ERROR: Not implemented!" << std::endl;
        exit(EXIT_FAILURE);
    };

    virtual void deleterFunction(const T& resource) {

        std::cout << "Resource [" << resource.getResourceType() << "]: " << resource.getResourceName() << " (cID: " << resource.getCID() << " | gID: " << resource.getGID()  << ")" << " freed" << std::endl;
        nameToIdMap_.erase(resource.getResourceName());
        textureMap_.erase(resource.getCID());
    }

    uint32_t assignCID();

protected:

    struct ResourceDeleter {
        void operator()(const T* t) const {
            if (t) {
                ResourceManager<T>::getInstance()->deleterFunction(*t);
                delete t;
            }
        }
    };

    std::shared_ptr<T>  get(std::string_view resourceName);
    std::shared_ptr<T>  get(uint32_t resourceId);
    ResourceManager() = default;
    ~ResourceManager() = default;

    constexpr inline static const char* assetPathPrefix{"../assets/"};

    inline static ResourceManager<T>* instance_{nullptr};

    uint32_t cidCounter_{0}; // id = 0 is reserved as invalid id
    std::mutex cidMutex_;

    std::unordered_map<uint32_t,std::weak_ptr<T>> textureMap_{};
    std::unordered_map<std::string,uint32_t> nameToIdMap_{};
};

template<ManagedResourceConcept T>
std::shared_ptr<T> ResourceManager<T>::get(uint32_t resourceId) {
    auto it = textureMap_.find(resourceId);
    if (it != textureMap_.end()) {
        return it->second.lock();
    }
    return nullptr;
}

template<ManagedResourceConcept T>
std::shared_ptr<T> ResourceManager<T>::get(std::string_view resourceName) {
    auto it = nameToIdMap_.find(std::string{resourceName});
    if (it != nameToIdMap_.end()) {
        return get(it->second);
    }
    return nullptr;
}

template<ManagedResourceConcept T>
uint32_t ResourceManager<T>::assignCID() {
    std::lock_guard<std::mutex> lock(cidMutex_);
    uint32_t newId = ++cidCounter_;

    if (newId == std::numeric_limits<uint32_t>::min()){
        std::cerr << "ERROR: invalid CID reached!" << std::endl;
        exit(EXIT_FAILURE);
    }

    return newId;
}
