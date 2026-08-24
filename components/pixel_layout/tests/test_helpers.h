#pragma once

#include "pixel_layout.h"

#include <cstdint>
#include <vector>

namespace esphome {
namespace pixel_layout {
namespace test {

inline Color rgb_at(const uint8_t *buf, int width, int x, int y) {
  const int i = (y * width + x) * 3;
  return Color(buf[i], buf[i + 1], buf[i + 2]);
}

inline int nonzero_count(const uint8_t *buf, int width, int height) {
  int n = 0;
  const int pixels = width * height;
  for (int i = 0; i < pixels; i++) {
    if (buf[i * 3] != 0 || buf[i * 3 + 1] != 0 || buf[i * 3 + 2] != 0)
      n++;
  }
  return n;
}

class ProbeWidget : public Widget {
 public:
  void draw(DrawContext & /*ctx*/, uint32_t /*now_ms*/) override {}
  void start_anim(uint32_t now_ms) { this->ensure_anim_started_(now_ms); }
  uint8_t opacity_at(uint32_t now_ms, uint8_t base) { return this->anim_opacity_(now_ms, base); }
  float amount_at(uint32_t now_ms) { return this->anim_amount_(now_ms); }
  void offset_at(uint32_t now_ms, int *dx, int *dy) { this->anim_offset_(now_ms, dx, dy); }
};

class ProbeCustom : public CustomWidget {
 public:
  uint8_t index_at(int x, int y) const { return this->index_at_(x, y); }
  Color color_for_index(uint8_t idx) const { return this->color_for_index_(idx); }
};

inline font::Font make_block_font() {
  static const uint8_t kData[8] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  font::Font font;
  font.set_bpp(1);
  font.set_height(8);
  font::Glyph glyph;
  glyph.codepoint = static_cast<uint32_t>('A');
  glyph.data = kData;
  glyph.width = 8;
  glyph.height = 8;
  glyph.advance = 8;
  font.add_glyph(glyph);
  return font;
}

}  // namespace test
}  // namespace pixel_layout
}  // namespace esphome
