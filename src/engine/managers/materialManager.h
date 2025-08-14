//
// Created by Tonz on 29.07.2025.
//

#pragma once
#include "resourceManager.h"
#include "../../scene/material.h"

class MaterialManager : public ResourceManager<Material> {
public:

    static MaterialManager* getInstance() {
        if (instance_ == nullptr) {
            instance_ = new MaterialManager();
        }
        return dynamic_cast<MaterialManager*>(instance_);
    }


    std::shared_ptr<Material> getResource(std::string_view resourceName) override {

        if (auto existingResource = get(resourceName))
            return existingResource;

        auto newResource = std::shared_ptr<Material>(new Material(), ResourceDeleter{});
        uint32_t newId = assignCID();
        newResource->setCID(newId);
        newResource->setGID(ResourceManagerBase::getInstance()->assignGlobalId());
        newResource->setResourceName(resourceName);

        nameToIdMap_[std::string{resourceName}] = newId;
        textureMap_[newId] = newResource;

        return newResource;
    }

    /*std::shared_ptr<Material> registerResource(const Material& material) {
        if (auto existingResource = get(material.getResourceName()))
            return existingResource;

        auto newResource = std::shared_ptr<Material>(new Material(material), ResourceDeleter{});
        uint32_t newId = assignCID();
        newResource->setCID(newId);

        nameToIdMap_[material.getResourceName()] = newId;
        textureMap_[newId] = newResource;

        return newResource;
    }*/

private:
    MaterialManager() = default;
};