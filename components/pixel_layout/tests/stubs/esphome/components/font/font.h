#pragma once

#include <cstdint>
#include <cstring>
#include <vector>

namespace esphome {
namespace font {

struct Glyph {
  uint32_t codepoint{0};
  const uint8_t *data{nullptr};
  int offset_x{0};
  int offset_y{0};
  int width{0};
  int height{0};
  int advance{0};
};

class Font {
 public:
  void add_glyph(const Glyph &glyph) { this->glyphs_.push_back(glyph); }
  void set_bpp(uint8_t bpp) { this->bpp_ = bpp; }
  void set_height(int height) { this->height_ = height; }

  uint8_t get_bpp() const { return this->bpp_; }
  int get_height() const { return this->height_; }
  const std::vector<Glyph> &get_glyphs() const { return this->glyphs_; }

  const Glyph *find_glyph(uint32_t codepoint) const {
    for (const auto &glyph : this->glyphs_) {
      if (glyph.codepoint == codepoint)
        return &glyph;
    }
    return nullptr;
  }

  static uint32_t next_codepoint(const char *&cursor) {
    if (cursor == nullptr || *cursor == '\0')
      return 0;
    const uint8_t c1 = static_cast<uint8_t>(*cursor);
    uint32_t code_point = c1;
    size_t length = 1;
    if (c1 < 0x80) {
      length = 1;
    } else if ((c1 & 0xE0) == 0xC0) {
      code_point = (c1 & 0x1F) << 6 | (static_cast<uint8_t>(cursor[1]) & 0x3F);
      length = 2;
    } else if ((c1 & 0xF0) == 0xE0) {
      code_point = (c1 & 0x0F) << 12 | (static_cast<uint8_t>(cursor[1]) & 0x3F) << 6 |
                   (static_cast<uint8_t>(cursor[2]) & 0x3F);
      length = 3;
    } else if ((c1 & 0xF8) == 0xF0) {
      code_point = (c1 & 0x07) << 18 | (static_cast<uint8_t>(cursor[1]) & 0x3F) << 12 |
                   (static_cast<uint8_t>(cursor[2]) & 0x3F) << 6 | (static_cast<uint8_t>(cursor[3]) & 0x3F);
      length = 4;
    }
    cursor += length;
    return code_point;
  }

  void measure(const char *text, int *width, int *x_off, int *baseline, int *height) const {
    *x_off = 0;
    *baseline = this->height_;
    *height = this->height_;
    int w = 0;
    if (text != nullptr) {
      const char *p = text;
      while (*p != '\0') {
        const uint32_t cp = next_codepoint(p);
        const Glyph *glyph = this->find_glyph(cp);
        w += glyph != nullptr ? glyph->advance : (this->glyphs_.empty() ? 0 : this->glyphs_[0].advance);
      }
    }
    *width = w;
  }

 protected:
  std::vector<Glyph> glyphs_{};
  uint8_t bpp_{1};
  int height_{8};
};

}  // namespace font
}  // namespace esphome
