#pragma once

#include "esphome/core/color.h"

#include <cstdint>
#include <vector>

namespace esphome {
namespace image {

class Image {
 public:
  Image() = default;
  Image(int width, int height) : width_(width), height_(height), pixels_(static_cast<size_t>(width * height)) {}

  void set_size(int width, int height) {
    this->width_ = width;
    this->height_ = height;
    this->pixels_.assign(static_cast<size_t>(width * height), Color());
  }

  void set_pixel(int x, int y, Color color) {
    if (x < 0 || y < 0 || x >= this->width_ || y >= this->height_)
      return;
    if (color.w == 0 && (color.r || color.g || color.b))
      color.w = 255;
    this->pixels_[static_cast<size_t>(y * this->width_ + x)] = color;
  }

  Color get_pixel(int x, int y, Color /*color_on*/, Color color_off) const {
    if (x < 0 || y < 0 || x >= this->width_ || y >= this->height_)
      return color_off;
    return this->pixels_[static_cast<size_t>(y * this->width_ + x)];
  }

  int get_width() const { return this->width_; }
  int get_height() const { return this->height_; }

 protected:
  int width_{0};
  int height_{0};
  std::vector<Color> pixels_{};
};

}  // namespace image
}  // namespace esphome
