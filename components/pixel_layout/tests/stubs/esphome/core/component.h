#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <utility>

namespace esphome {

namespace setup_priority {
constexpr float HARDWARE = 50.0f;
constexpr float PROCESSOR = 200.0f;
}  // namespace setup_priority

class Component {
 public:
  virtual ~Component() = default;
  virtual void setup() {}
  virtual void dump_config() {}
  virtual float get_setup_priority() const { return setup_priority::PROCESSOR; }

  void mark_failed() { this->failed_ = true; }
  bool is_failed() const { return this->failed_; }

  template<typename F> void set_timeout(uint32_t /*timeout*/, F && /*f*/) {}
  template<typename F> void set_timeout(const std::string & /*name*/, uint32_t /*timeout*/, F && /*f*/) {}
  template<typename F> void set_interval(const std::string & /*name*/, uint32_t /*interval*/, F && /*f*/) {}

 protected:
  bool failed_{false};
};

}  // namespace esphome
