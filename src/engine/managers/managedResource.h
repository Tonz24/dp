//
// Created by Tonz on 28.07.2025.
//

#pragma once
#include <cstdint>
#include <string>

class ManagedResource;

template <typename T>
concept ManagedResourceConcept = std::is_base_of_v<ManagedResource,T>;
template <ManagedResourceConcept T, typename Derived>
class ResourceManager;

class ManagedResource {
public:

    virtual ~ManagedResource() = default;

    [[nodiscard]] virtual uint32_t getCID() const {
        return categoryId_;
    };

    [[nodiscard]] virtual uint32_t getGID() const {
        return globalId_;
    };

    [[nodiscard]] virtual std::string getFullFileName() const {
        return fileName_ + extension_;
    };

    [[nodiscard]] virtual std::string getResourceName() const {
        return resourceName_;
    };


    [[nodiscard]] bool isValid() const{
        return  categoryId_ != 0 && !resourceName_.empty() && globalId_ != 0;
    }

    [[nodiscard]] virtual std::string getResourceType() const = 0;


    template <ManagedResourceConcept T, typename Derived>
    friend class ResourceManager;

protected:
    ManagedResource() = default;

    uint32_t globalId_{0};
    uint32_t categoryId_{0};

    std::string resourceName_;
    std::string fileName_;
    std::string extension_;

    bool isFromDisk_{false};
    bool isRegistered_{false};
};
