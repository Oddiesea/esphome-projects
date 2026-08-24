#pragma once

#include <cstdint>

namespace esphome {

inline uint32_t &host_millis() {
  static uint32_t value = 0;
  return value;
}

inline uint32_t millis() { return host_millis(); }
inline void set_millis(uint32_t value) { host_millis() = value; }

}  // namespace esphome
