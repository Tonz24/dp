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

    virtual std::shared_ptr<T>  getResource(std::string_view resourceName) {
        std::cerr << "ERROR: Not implemented!" << std::endl;
        exit(EXIT_FAILURE);
    }

    virtual std::shared_ptr<T>  getR(std::string_view resourceName) {
        if (auto existingResource = get(resourceName))
            return existingResource;

        return nullptr;
    };

    virtual std::shared_ptr<T> registerResource(T* mesh, std::string_view resourceName) {

        if (auto existingResource = get(resourceName)) {
            std::cerr << "ERROR: Resource with name " << resourceName << " is already registered!" << std::endl;
            exit(EXIT_FAILURE);
        }

        auto newResource = std::shared_ptr<T>(mesh, ResourceDeleter{});

        uint32_t newCID = assignCID();
        uint32_t newGID = ResourceManagerBase::getInstance()->assignGlobalId();
        newResource->setCID(newCID);
        newResource->setGID(newGID);
        newResource->setResourceName(resourceName);
        newResource->isRegistered_ = true;

        nameToIdMap_[std::string{resourceName}] = newCID;
        idToResourceMap_[newCID] = newResource;

        return newResource;
    }

    virtual void deleterFunction(const T& resource) {

        std::cout << "Resource [" << resource.getResourceType() << "]: " << resource.getResourceName() << " (cID: " << resource.getCID() << " | gID: " << resource.getGID()  << ")" << " freed" << std::endl;
        nameToIdMap_.erase(resource.getResourceName());
        idToResourceMap_.erase(resource.getCID());
    }

    uint32_t assignCID() {
        std::lock_guard<std::mutex> lock(cidMutex_);
        uint32_t newId = ++cidCounter_;

        if (newId == std::numeric_limits<uint32_t>::min()){
            std::cerr << "ERROR: invalid CID reached!" << std::endl;
            exit(EXIT_FAILURE);
        }

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
    std::shared_ptr<T>  get(std::string_view resourceName) {
        auto it = nameToIdMap_.find(std::string{resourceName});
        if (it != nameToIdMap_.end()) {
            return get(it->second);
        }
        return nullptr;
    }

    std::shared_ptr<T>  get(uint32_t resourceId) {
        auto it = idToResourceMap_.find(resourceId);
        if (it != idToResourceMap_.end()) {
            return it->second.lock();
        }
        return nullptr;
    }
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
