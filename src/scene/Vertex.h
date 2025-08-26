//
// Created by Tonz on 29.07.2025.
//

#pragma once
#include <glm/glm.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <array>

struct Vertex3D{
    glm::vec3 position{};
    glm::vec3 normal{};
    glm::vec3 tangent{};

    glm::vec2 texCoord{};

    static constexpr vk::VertexInputBindingDescription getBindingDescription(){
        return vk::VertexInputBindingDescription{
            .binding = 0,
            .stride = sizeof(Vertex3D),
            .inputRate = vk::VertexInputRate::eVertex
        };
    }

    static constexpr std::array<vk::VertexInputAttributeDescription, 4> getAttributeDescriptions() {
        return {
            vk::VertexInputAttributeDescription{ // position
                .location = 0,
                .binding = 0,
                .format = vk::Format::eR32G32B32Sfloat,
                .offset = static_cast<uint32_t>(offsetof(Vertex3D, position))
            },
            vk::VertexInputAttributeDescription{ // normal
                .location = 1,
                .binding = 0,
                .format = vk::Format::eR32G32B32Sfloat,
                .offset = static_cast<uint32_t>(offsetof(Vertex3D, normal))
            },
            vk::VertexInputAttributeDescription{ // tangent
                .location = 2,
                .binding = 0,
                .format = vk::Format::eR32G32B32Sfloat,
                .offset = static_cast<uint32_t>(offsetof(Vertex3D, tangent))
            },
            vk::VertexInputAttributeDescription{ // texCoord
                .location = 3,
                .binding = 0,
                .format = vk::Format::eR32G32Sfloat,
                .offset = static_cast<uint32_t>(offsetof(Vertex3D, texCoord))
            }
        };
    }
};

struct Vertex2D{
    glm::vec2 position{};
    glm::vec2 texCoord{};

    static constexpr vk::VertexInputBindingDescription getBindingDescription(){
        return vk::VertexInputBindingDescription{
            .binding = 0,
            .stride = sizeof(Vertex2D),
            .inputRate = vk::VertexInputRate::eVertex
        };
    }

    static constexpr std::array<vk::VertexInputAttributeDescription, 2> getAttributeDescriptions() {
        return {
            vk::VertexInputAttributeDescription{ // position
                .location = 0,
                .binding = 0,
                .format = vk::Format::eR32G32Sfloat,
                .offset = static_cast<uint32_t>(offsetof(Vertex2D, position))
            },
            vk::VertexInputAttributeDescription{ // position
                .location = 1,
                .binding = 0,
                .format = vk::Format::eR32G32Sfloat,
                .offset = static_cast<uint32_t>(offsetof(Vertex2D, texCoord))
            }
        };
    }
};