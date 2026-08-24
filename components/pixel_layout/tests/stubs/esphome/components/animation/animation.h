#pragma once

#include "esphome/components/image/image.h"

namespace esphome {
namespace animation {

class Animation : public image::Image {
 public:
  using image::Image::Image;
  void next_frame() { this->frame_++; }
  int frame() const { return this->frame_; }

 protected:
  int frame_{0};
};

}  // namespace animation
}  // namespace esphome
