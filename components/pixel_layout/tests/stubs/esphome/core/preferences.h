#pragma once

#include <cstdint>
#include <cstring>

namespace esphome {

template<typename T> class ESPPreferenceObject {
 public:
  bool load(T *dest) {
    if (!this->valid_)
      return false;
    *dest = this->data_;
    return true;
  }
  void save(const T *src) {
    this->data_ = *src;
    this->valid_ = true;
  }

 protected:
  T data_{};
  bool valid_{false};
};

class ESPPreferences {
 public:
  template<typename T> ESPPreferenceObject<T> make_preference(uint32_t /*hash*/) { return ESPPreferenceObject<T>(); }
};

inline ESPPreferences *global_preferences = nullptr;

inline uint32_t fnv1_hash(const char *str) {
  uint32_t h = 2166136261u;
  for (const char *p = str; p != nullptr && *p; ++p) {
    h ^= static_cast<uint8_t>(*p);
    h *= 16777619u;
  }
  return h;
}

}  // namespace esphome
