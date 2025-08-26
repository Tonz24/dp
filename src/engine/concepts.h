//
// Created by Tonz on 26.08.2025.
//

#pragma once
#include "../scene/Vertex.h"

template<typename T>
concept VertexType = requires {
    { T::getBindingDescription() } -> std::same_as<vk::VertexInputBindingDescription>;
    { T::getAttributeDescriptions()};
};


