//
// Created by Tonz on 29.07.2025.
//

#pragma once
#include <glm/glm.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <array>

struct Vertex{

    glm::vec3 position{};
    glm::vec3 normal{};
    glm::vec3 tangent{};

    glm::vec2 texCoord{};

    static vk::VertexInputBindingDescription getBindingDescription(){
        return vk::VertexInputBindingDescription{
            .binding = 0,
            .stride = sizeof(Vertex),
            .inputRate = vk::VertexInputRate::eVertex
        };
    }

    static std::array<vk::VertexInputAttributeDescription, 4> getAttributeDescriptions() {
        return {
            vk::VertexInputAttributeDescription{ // position
                .location = 0,
                .binding = 0,
                .format = vk::Format::eR32G32B32Sfloat,
                .offset = static_cast<uint32_t>(offsetof(Vertex, position))
            },
            vk::VertexInputAttributeDescription{ // normal
                .location = 1,
                .binding = 0,
                .format = vk::Format::eR32G32B32Sfloat,
                .offset = static_cast<uint32_t>(offsetof(Vertex, normal))
            },
            vk::VertexInputAttributeDescription{ // tangent
                .location = 2,
                .binding = 0,
                .format = vk::Format::eR32G32B32Sfloat,
                .offset = static_cast<uint32_t>(offsetof(Vertex, tangent))
            },
            vk::VertexInputAttributeDescription{ // texCoord
                .location = 3,
                .binding = 0,
                .format = vk::Format::eR32G32Sfloat,
                .offset = static_cast<uint32_t>(offsetof(Vertex, texCoord))
            }
        };
    }
};