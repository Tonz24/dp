//
// Created by Tonz on 31.07.2025.
//

#pragma once

#include "resourceManager.h"
#include "../scene/mesh.h"

class MeshManager : public ResourceManager<Mesh> {
public:
    static MeshManager* getInstance() {
        if (instance_ == nullptr) {
            instance_ = new MeshManager();
        }
        return dynamic_cast<MeshManager*>(instance_);
    }


    std::shared_ptr<Mesh> getResource(std::string_view resourceName) override {

        if (auto existingResource = get(resourceName))
            return existingResource;

       return nullptr;
    }

    std::shared_ptr<Mesh> createResource(std::string_view resourceName, std::vector<Vertex>&& vertices,  std::vector<uint32_t>&& indices, std::shared_ptr<Material> material) {

        auto newResource = std::shared_ptr<Mesh>(new Mesh(std::move(vertices),std::move(indices),material), ResourceDeleter{});
        uint32_t newId = assignCID();
        newResource->setCID(newId);
        newResource->setGID(ResourceManagerBase::getInstance()->assignGlobalId());
        newResource->setResourceName(resourceName);

        nameToIdMap_[std::string{resourceName}] = newId;
        textureMap_[newId] = newResource;

        return newResource;
    }


private:
    MeshManager() = default;
};
