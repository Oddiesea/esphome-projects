#pragma once

namespace esphome {
namespace display {

enum ColorOrder {
  COLOR_ORDER_RGB = 0,
  COLOR_ORDER_BGR = 1,
};

enum ColorBitness {
  COLOR_BITNESS_888 = 0,
  COLOR_BITNESS_565 = 1,
};

}  // namespace display
}  // namespace esphome
