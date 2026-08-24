#pragma once

#include <cstdint>

namespace esphome {

class Color {
 public:
  uint8_t r{0};
  uint8_t g{0};
  uint8_t b{0};
  uint8_t w{0};

  Color() = default;
  Color(uint8_t red, uint8_t green, uint8_t blue, uint8_t white = 0) : r(red), g(green), b(blue), w(white) {}
};

}  // namespace esphome
