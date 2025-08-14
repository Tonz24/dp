//
// Created by Tonz on 28.07.2025.
//

#pragma once
#include <cstdint>
#include <string>

class ManagedResource {
public:

    virtual ~ManagedResource() = default;

    [[nodiscard]] virtual uint32_t getCID() const {
        return categoryId_;
    };

    virtual void setCID(uint32_t newId) {
        categoryId_ = newId;
    };

    [[nodiscard]] virtual uint32_t getGID() const {
        return globalId_;
    };

    void setGID(uint32_t newId) {
        globalId_ = newId;
    };

    [[nodiscard]] virtual std::string getFullFileName() const {
        return fileName_ + extension_;
    };

    [[nodiscard]] virtual std::string getResourceName() const {
        return resourceName_;
    };

    void setResourceName(std::string_view resourceName){
        resourceName_ = resourceName;
    }


    [[nodiscard]] bool isValid() const{
        //return globalId_ != 0 && categoryId_ != 0 && fileName_ != "" && extension_ != "";
        return  categoryId_ != 0 && !resourceName_.empty();
    }

    [[nodiscard]] virtual std::string getResourceType() const = 0;


protected:
    ManagedResource() = default;

    uint32_t globalId_{0};
    uint32_t categoryId_{0};

    std::string resourceName_;
    std::string fileName_;
    std::string extension_;

    bool isFromDisk_{false};
};
