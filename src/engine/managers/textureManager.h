//
// Created by Tonz on 29.07.2025.
//

#pragma once
#include "resourceManager.h"
#include "../texture.h"

class TextureManager : public ResourceManager<Texture> {
public:

    static TextureManager* getInstance() {
        if (instance_ == nullptr) {
            instance_ = new TextureManager();
        }
        return dynamic_cast<TextureManager*>(instance_);
    }


    std::shared_ptr<Texture> getResource(std::string_view resourceName) override {

        if (auto existingResource = get(resourceName))
            return existingResource;

        auto newResource = std::shared_ptr<Texture>(new Texture(resourceName), ResourceDeleter{});
        uint32_t newCID = assignCID();
        uint32_t newGID = ResourceManagerBase::getInstance()->assignGlobalId();
        newResource->setCID(newCID);
        newResource->setGID(newGID);
        newResource->setResourceName(resourceName);

        nameToIdMap_[std::string{resourceName}] = newCID;
        textureMap_[newCID] = newResource;

        return newResource;
    }

private:
    TextureManager() = default;
};
