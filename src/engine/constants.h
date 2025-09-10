//
// Created by Tonz on 03.09.2025.
//

#pragma once
#include <cstdint>

class Constants{
public:
  Constants() = delete;

  static constexpr uint32_t maxFramesInFlight{1};
  static constexpr uint32_t materialLimit{100};
  static constexpr uint32_t bindlessTextureLimit{1024};
  static constexpr uint32_t textureSamplerLimit{1024};


  #ifdef NDEBUG
  static constexpr bool enableValidationLayers{false};
  #else
  static constexpr bool enableValidationLayers{true};
  #endif
};
