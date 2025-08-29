//
// Created by Tonz on 28.07.2025.
//

#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <iostream>

#include "managedResource.h"
#include "resourceManagerBase.h"
#include "../gBuffer.h"
#include "../../scene/material.h"
#include "../../scene/mesh.h"


template <ManagedResourceConcept T, typename Derived>
class ResourceManager : public ResourceManagerBase {
public:

    static Derived* getInstance(){
        if (instance_ == nullptr)
            instance_ = new Derived();
        return static_cast<Derived*>(instance_);
    }

    std::shared_ptr<T>  getResource(std::string_view resourceName) {
        auto it = nameToIdMap_.find(std::string{resourceName});
        if (it != nameToIdMap_.end()) {
            return getResource(it->second);
        }
        return nullptr;
    }

    std::shared_ptr<T>  getResource(uint32_t resourceId) {
        auto it = idToResourceMap_.find(resourceId);
        if (it != idToResourceMap_.end()) {
            return it->second.lock();
        }
        return nullptr;
    }

    std::shared_ptr<T> registerResource(T* resource, std::string_view resourceName) {

        if (getResource(resourceName) != nullptr)
            throw std::runtime_error("ERROR: Resource with name " + std::string{resourceName} + " is already registered!");

        auto newResource = std::shared_ptr<T>(resource, ResourceDeleter{});

        uint32_t newCID = assignCategoryId();
        uint32_t newGID = ResourceManagerBase::getInstance()->assignGlobalId();
        newResource->categoryId_ = newCID;
        newResource->globalId_ = newGID;
        newResource->resourceName_ = resourceName;
        newResource->isRegistered_ = true;

        nameToIdMap_[std::string{resourceName}] = newCID;
        idToResourceMap_[newCID] = newResource;

        return newResource;
    }

    template <typename... Args>
    std::shared_ptr<T> registerResource(std::string_view resourceName, Args&&... args) {

        if (getResource(resourceName) != nullptr)
            throw std::runtime_error("ERROR: Resource with name " + std::string{resourceName} + " is already registered!");

        auto newResource = std::shared_ptr<T>(new T(std::forward<Args>(args)...), ResourceDeleter{});

        uint32_t newCId = assignCategoryId();
        uint32_t newGId = ResourceManagerBase::getInstance()->assignGlobalId();
        newResource->categoryId_ = newCId;
        newResource->globalId_ = newGId;
        newResource->resourceName_ = resourceName;
        newResource->isRegistered_ = true;

        nameToIdMap_[std::string{resourceName}] = newCId;
        idToResourceMap_[newCId] = newResource;

        return newResource;
    }

    void deleterFunction(const T& resource) {

        std::cout << "Resource [" << resource.getResourceType() << "]: " << resource.getResourceName() << " (cID: " << resource.getCID() << " | gID: " << resource.getGID()  << ")" << " freed" << std::endl;
        nameToIdMap_.erase(resource.getResourceName());
        idToResourceMap_.erase(resource.getCID());
    }

    uint32_t assignCategoryId() {
        std::lock_guard<std::mutex> lock(cidMutex_);
        uint32_t newId = ++cidCounter_;

        if (newId == std::numeric_limits<uint32_t>::min())
            throw std::runtime_error("ERROR: invalid CID reached!");

        return newId;
    }


    struct ResourceDeleter {
        void operator()(const T* t) const {
            if (t) {
                Derived::getInstance()->deleterFunction(*t);
                delete t;
            }
        }
    };

protected:

    ResourceManager() = default;

    constexpr static const char* assetPathPrefix{"../assets/"};

    inline static ResourceManagerBase* instance_{nullptr};

    uint32_t cidCounter_{0}; // id = 0 is reserved as invalid id
    std::mutex cidMutex_;

    std::unordered_map<uint32_t,std::weak_ptr<T>> idToResourceMap_{};
    std::unordered_map<std::string,uint32_t> nameToIdMap_{};
};

class Mesh;
class Texture;
class Material;
class GBuffer;


class MeshManager : public ResourceManager<Mesh, MeshManager> {
    friend class ResourceManager<Mesh, MeshManager>;
    MeshManager() = default;
};

class TextureManager : public ResourceManager<Texture, TextureManager> {
    friend class ResourceManager<Texture, TextureManager>;
    TextureManager() = default;
};

class MaterialManager : public ResourceManager<Material, MaterialManager> {
    friend class ResourceManager<Material, MaterialManager>;
    MaterialManager() = default;
};

class GBufferManager : public ResourceManager<GBuffer, GBufferManager> {
    friend class ResourceManager<GBuffer, GBufferManager>;
    GBufferManager() = default;

public:

    template <typename... Args>
    std::shared_ptr<GBuffer> registerResource(std::string_view resourceName, Args&&... args) {

        if (getResource(resourceName) != nullptr)
            throw std::runtime_error("ERROR: Resource with name " + std::string{resourceName} + " is already registered!");

        auto newResource = std::shared_ptr<GBuffer>(new GBuffer(resourceName, std::forward<Args>(args)...), ResourceDeleter{});

        uint32_t newCId = assignCategoryId();
        uint32_t newGId = ResourceManagerBase::getInstance()->assignGlobalId();
        newResource->categoryId_ = newCId;
        newResource->globalId_ = newGId;
        newResource->resourceName_ = resourceName;
        newResource->isRegistered_ = true;

        nameToIdMap_[std::string{resourceName}] = newCId;
        idToResourceMap_[newCId] = newResource;

        return newResource;
    }
};