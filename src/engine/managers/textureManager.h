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


    std::shared_ptr<Texture> getR(std::string_view resourceName){

        if (auto existingResource = get(resourceName))
            return existingResource;

        return nullptr;
    }


    void registerResource(const std::shared_ptr<Texture>& texture, std::string_view resourceName) {

        if (auto existingResource = get(resourceName)) {
            std::cerr << "ERROR: Resource with name " << resourceName << " is already registered!" << std::endl;
            exit(EXIT_FAILURE);
        }

        auto newResource = texture;

        uint32_t newCID = assignCID();
        uint32_t newGID = ResourceManagerBase::getInstance()->assignGlobalId();
        newResource->setCID(newCID);
        newResource->setGID(newGID);
        newResource->setResourceName(resourceName);
        newResource->isRegistered_ = true;

        nameToIdMap_[std::string{resourceName}] = newCID;
        textureMap_[newCID] = newResource;
    }

private:
    TextureManager() = default;
};
