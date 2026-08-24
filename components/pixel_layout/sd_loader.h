#pragma once

#include <memory>
#include <string>
#include <vector>

namespace esphome {
namespace pixel_layout {

class PixelLayout;
class Widget;

/** Maximum screens in SD playlist and HA screen slots. */
static constexpr size_t kMaxScreenSlots = 32;

struct SdScreenSpec {
  std::string id;
  Widget *root{nullptr};
  uint32_t duration_ms{0};
  ScreenTransition transition{ScreenTransition::FADE};
  uint32_t transition_ms{0};
};

#ifdef USE_PIXEL_LAYOUT_SD_STORAGE

#ifdef USE_IMAGE
namespace esphome {
namespace image {
class Image;
}  // namespace image
}  // namespace esphome
#endif

struct SdOwnedImage {
  std::vector<uint8_t> data;
#ifdef USE_IMAGE
  esphome::image::Image *image{nullptr};
  ~SdOwnedImage();
#endif
};

/** Runtime playlist loader: parse playlist.json from SD and build widget trees. */
class SdPlaylistLoader {
 public:
  explicit SdPlaylistLoader(PixelLayout *host) : host_(host) {}

  bool load(const std::string &root_path, std::string *err);
  void take_ownership(std::vector<std::unique_ptr<Widget>> widgets, std::vector<SdOwnedImage> images);

 protected:
  PixelLayout *host_;
};

#endif

}  // namespace pixel_layout
}  // namespace esphome
