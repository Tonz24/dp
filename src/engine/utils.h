//
// Created by Tonz on 23.07.2025.
//

#pragma once
#include <string>
#include <vector>

#include <glm/glm.hpp>


class Utils {
public:
    Utils() = delete; //static class
    static std::vector<char> readFile(const std::string& filename);

    static glm::vec3 expand(const glm::vec3& u){
        return {expand(u.x), expand(u.y), expand(u.z)};
    }


    static float expand(float u) {
        if (u <= 0.0f)
            u = 0.0f;
        else if (u >= 1.0f)
            u = 1.0f;
        else if (u <= 0.04045f)
            u /=  12.92f;
        else
            u = powf((u + 0.055f) / 1.055f, 2.4f);
        return u;

    }

};
