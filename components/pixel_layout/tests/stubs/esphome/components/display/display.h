#pragma once

#include "esphome/core/color.h"
#include "esphome/components/display/display_color_utils.h"

#include <cstdint>
#include <functional>
#include <vector>

namespace esphome {
namespace display {

class Display {
 public:
  Display() = default;
  Display(int width, int height) : width_(width), height_(height) {}

  void set_size(int width, int height) {
    this->width_ = width;
    this->height_ = height;
  }

  int get_width() const { return this->width_; }
  int get_height() const { return this->height_; }

  void set_auto_clear(bool /*clear*/) {}
  void stop_poller() {}

  void set_writer(std::function<void(Display &)> writer) { this->writer_ = std::move(writer); }
  bool has_writer() const { return static_cast<bool>(this->writer_); }
  void invoke_writer() {
    if (this->writer_)
      this->writer_(*this);
  }
  void update() { this->invoke_writer(); }

  void draw_pixels_at(int /*x_start*/, int /*y_start*/, int w, int h, const uint8_t *ptr, ColorOrder order,
                      ColorBitness bitness, bool big_endian) {
    this->last_w_ = w;
    this->last_h_ = h;
    this->last_order_ = order;
    this->last_bitness_ = bitness;
    this->last_big_endian_ = big_endian;
    const size_t n = static_cast<size_t>(w) * static_cast<size_t>(h) * 3;
    this->captured_.assign(ptr, ptr + n);
  }

  const std::vector<uint8_t> &captured() const { return this->captured_; }
  int last_w() const { return this->last_w_; }
  int last_h() const { return this->last_h_; }
  ColorOrder last_order() const { return this->last_order_; }
  ColorBitness last_bitness() const { return this->last_bitness_; }
  bool last_big_endian() const { return this->last_big_endian_; }

 protected:
  int width_{0};
  int height_{0};
  std::function<void(Display &)> writer_{};
  std::vector<uint8_t> captured_{};
  int last_w_{0};
  int last_h_{0};
  ColorOrder last_order_{COLOR_ORDER_RGB};
  ColorBitness last_bitness_{COLOR_BITNESS_888};
  bool last_big_endian_{false};
};

}  // namespace display
}  // namespace esphome
