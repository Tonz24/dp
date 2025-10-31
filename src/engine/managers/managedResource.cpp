//
// Created by Tonz on 28.07.2025.
//

#include "managedResource.h"

#include <iostream>

ManagedResource::~ManagedResource() {
    if (categoryId_ != 0 && recycleFunc != nullptr && isRegistered_) {
        recycleFunc(categoryId_);
        categoryId_ = 0;
        isRegistered_ = false;
    }
}

void ManagedResource::printDeleteMessage() const {
    std::cout << "Resource [" << getResourceType() << "]: " << getResourceName() << " (cID: " << getCID() << " | gID: " << getGID()  << ")" << " deleted" << std::endl;
}
