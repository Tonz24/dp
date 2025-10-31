//
// Created by Tonz on 03.09.2025.
//

#pragma once
#include <cstdint>
#include <string>

class Constants{
public:
  Constants() = delete;

  static constexpr uint32_t maxFramesInFlight{1};
  static constexpr uint32_t materialLimit{100};
  static constexpr uint32_t objDescLimit{100};
  static constexpr uint32_t bindlessTextureLimit{1024};
  static constexpr uint32_t bindlessTextureUintLimit{1024};
  static constexpr uint32_t textureSamplerLimit{1024};

  static constexpr uint32_t defaultCategoryIdLimit{1000};


  #ifdef NDEBUG
  static constexpr bool enableValidationLayers{false};
  #else
  static constexpr bool enableValidationLayers{true};
  #endif
};


class Paths{
public:
  Paths() = delete;

  static inline const std::string exportPrefix{"../exports/"};
  static inline const std::string scenePrefix{"../assets/models/"};
  static inline const std::string envmapPrefix{"../assets/envmaps/"};

};
