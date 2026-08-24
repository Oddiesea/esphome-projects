#pragma once

#include <cmath>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace esphome {
namespace sensor {

class Sensor {
 public:
  float state{NAN};
  bool has_state() const { return this->has_state_; }
  float get_state() const { return this->state; }
  std::string get_unit_of_measurement() const { return this->unit_; }
  void set_unit_of_measurement(const std::string &unit) { this->unit_ = unit; }
  void set_state(float value) {
    this->state = value;
    this->has_state_ = true;
    for (auto &cb : this->callbacks_)
      cb(value);
  }
  void clear_state() {
    this->has_state_ = false;
    this->state = NAN;
  }
  void add_on_state_callback(std::function<void(float)> cb) { this->callbacks_.push_back(std::move(cb)); }

 protected:
  bool has_state_{false};
  std::string unit_{};
  std::vector<std::function<void(float)>> callbacks_{};
};

}  // namespace sensor
}  // namespace esphome
