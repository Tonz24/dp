//
// Created by Tonz on 23.07.2025.
//

#include "utils.h"
#include <fstream>
#include <iostream>

std::vector<char> Utils::readFile(const std::string &filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open())
        throw std::runtime_error("failed to open file!");

    size_t fileSize = file.tellg();
    std::vector<char> fileBuf(fileSize);

    file.seekg(0);
    file.read(fileBuf.data(), fileSize);
    file.close();

    return fileBuf;
}
