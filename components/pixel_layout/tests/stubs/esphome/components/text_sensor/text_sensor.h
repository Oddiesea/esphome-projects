#pragma once

#include <functional>
#include <string>
#include <vector>

namespace esphome {
namespace text_sensor {

class TextSensor {
 public:
  std::string state;
  bool has_state() const { return this->has_state_; }
  void set_state(const std::string &value) {
    this->state = value;
    this->has_state_ = true;
    for (auto &cb : this->callbacks_)
      cb(value);
  }
  void clear_state() { this->has_state_ = false; }
  void add_on_state_callback(std::function<void(const std::string &)> cb) { this->callbacks_.push_back(std::move(cb)); }

 protected:
  bool has_state_{false};
  std::vector<std::function<void(const std::string &)>> callbacks_{};
};

}  // namespace text_sensor
}  // namespace esphome
