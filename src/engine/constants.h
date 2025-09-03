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
};
