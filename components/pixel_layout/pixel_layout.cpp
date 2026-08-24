#include "pixel_layout.h"

#include "esphome/core/hal.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"
#include "esphome/core/preferences.h"
#include "esphome/components/display/display_color_utils.h"

#include <cinttypes>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace esphome {
namespace pixel_layout {

static const char *const TAG = "pixel_layout";

static uint8_t blend_channel(uint8_t dst, uint8_t src, uint8_t alpha) {
  return static_cast<uint8_t>((src * alpha + dst * (255 - alpha) + 127) / 255);
}

static uint8_t resolve_src_alpha(Color color, uint8_t alpha, uint8_t alpha_scale) {
  if (alpha == 0)
    return 0;
  if (alpha_scale != 255)
    alpha = static_cast<uint8_t>((uint16_t) alpha * alpha_scale / 255);
  if (alpha == 0)
    return 0;
  return color.w != 0 ? static_cast<uint8_t>((uint16_t) alpha * color.w / 255) : alpha;
}

static void store_rgb(uint8_t *p, Color color) {
  p[0] = color.r;
  p[1] = color.g;
  p[2] = color.b;
}

static void blend_rgb(uint8_t *p, Color color, uint8_t src_a) {
  if (src_a >= 255) {
    store_rgb(p, color);
    return;
  }
  p[0] = blend_channel(p[0], color.r, src_a);
  p[1] = blend_channel(p[1], color.g, src_a);
  p[2] = blend_channel(p[2], color.b, src_a);
}

void DrawContext::init(uint8_t *buffer, int width, int height) {
  this->buffer_ = buffer;
  this->width_ = width;
  this->height_ = height;
  this->origin_x_ = 0;
  this->origin_y_ = 0;
  this->alpha_scale_ = 255;
  this->reset_mask();
}

void DrawContext::reset_mask() {
  this->clip_ = false;
  this->clip_x0_ = 0;
  this->clip_y0_ = 0;
  this->clip_x1_ = 0;
  this->clip_y1_ = 0;
  this->dissolve_ = false;
  this->dissolve_limit_ = 0;
  this->blinds_period_ = 0;
  this->blinds_open_ = 0;
  this->blinds_vertical_ = false;
}

void DrawContext::set_clip(int x, int y, int w, int h) {
  this->clip_ = true;
  if (w <= 0 || h <= 0) {
    this->clip_x0_ = 0;
    this->clip_y0_ = 0;
    this->clip_x1_ = 0;
    this->clip_y1_ = 0;
    return;
  }
  this->clip_x0_ = std::max(0, x);
  this->clip_y0_ = std::max(0, y);
  this->clip_x1_ = std::min(this->width_, x + w);
  this->clip_y1_ = std::min(this->height_, y + h);
}

void DrawContext::set_dissolve(uint16_t limit) {
  this->dissolve_ = true;
  this->dissolve_limit_ = limit;
}

void DrawContext::set_blinds(uint8_t open, uint8_t period, bool vertical) {
  this->blinds_period_ = period == 0 ? 8 : period;
  this->blinds_open_ = open;
  this->blinds_vertical_ = vertical;
}

static uint8_t pixel_hash(int x, int y) {
  uint32_t n = static_cast<uint32_t>(x) * 374761393u + static_cast<uint32_t>(y) * 668265263u;
  n = (n ^ (n >> 13)) * 1274126177u;
  return static_cast<uint8_t>(n >> 24);
}

bool DrawContext::accepts_(int x, int y) const {
  if (this->clip_ && (x < this->clip_x0_ || x >= this->clip_x1_ || y < this->clip_y0_ || y >= this->clip_y1_))
    return false;
  if (this->dissolve_ && pixel_hash(x, y) >= this->dissolve_limit_)
    return false;
  if (this->blinds_period_ != 0) {
    const int coord = this->blinds_vertical_ ? x : y;
    if (coord < 0 || (coord % static_cast<int>(this->blinds_period_)) >= this->blinds_open_)
      return false;
  }
  return true;
}

void DrawContext::clear(Color background) {
  if (this->buffer_ == nullptr)
    return;
  uint8_t *p = this->buffer_;
  const size_t bytes = static_cast<size_t>(this->width_) * static_cast<size_t>(this->height_) * 3;
  if (background.r == background.g && background.g == background.b) {
    memset(p, background.r, bytes);
    return;
  }
  uint8_t *end = p + bytes;
  while (p != end) {
    store_rgb(p, background);
    p += 3;
  }
}

void DrawContext::blit(display::Display &it) const {
  if (this->buffer_ == nullptr)
    return;
  it.draw_pixels_at(0, 0, this->width_, this->height_, this->buffer_, display::COLOR_ORDER_RGB,
                    display::COLOR_BITNESS_888, false);
}

void DrawContext::blend_pixel(int x, int y, Color color, uint8_t alpha) {
  x += this->origin_x_;
  y += this->origin_y_;
  if (this->buffer_ == nullptr || x < 0 || y < 0 || x >= this->width_ || y >= this->height_)
    return;
  if (!this->accepts_(x, y))
    return;
  const uint8_t src_a = resolve_src_alpha(color, alpha, this->alpha_scale_);
  if (src_a == 0)
    return;
  blend_rgb(this->buffer_ + (y * this->width_ + x) * 3, color, src_a);
}

void DrawContext::fill_rect(int x, int y, int w, int h, Color color, uint8_t alpha) {
  if (this->buffer_ == nullptr || w <= 0 || h <= 0)
    return;
  const uint8_t src_a = resolve_src_alpha(color, alpha, this->alpha_scale_);
  if (src_a == 0)
    return;
  x += this->origin_x_;
  y += this->origin_y_;
  int x0 = std::max(x, 0);
  int y0 = std::max(y, 0);
  int x1 = std::min(x + w, this->width_);
  int y1 = std::min(y + h, this->height_);
  if (this->clip_) {
    x0 = std::max(x0, this->clip_x0_);
    y0 = std::max(y0, this->clip_y0_);
    x1 = std::min(x1, this->clip_x1_);
    y1 = std::min(y1, this->clip_y1_);
  }
  if (x0 >= x1 || y0 >= y1)
    return;
  uint8_t *row = this->buffer_ + (y0 * this->width_ + x0) * 3;
  const int stride = this->width_ * 3;
  const int span = x1 - x0;
  const bool gated = this->dissolve_ || this->blinds_period_ != 0;
  for (int yy = y0; yy < y1; yy++) {
    uint8_t *p = row;
    for (int i = 0; i < span; i++) {
      if (!gated || this->accepts_(x0 + i, yy))
        blend_rgb(p, color, src_a);
      p += 3;
    }
    row += stride;
  }
}

static float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

static float sdf_round_rect(float px, float py, float cx, float cy, float hw, float hh, float r) {
  const float dx = fabsf(px - cx) - (hw - r);
  const float dy = fabsf(py - cy) - (hh - r);
  const float ox = std::max(dx, 0.0f);
  const float oy = std::max(dy, 0.0f);
  return sqrtf(ox * ox + oy * oy) + std::min(std::max(dx, dy), 0.0f) - r;
}

void DrawContext::fill_round_rect(int x, int y, int w, int h, int radius, bool antialias, Color color, uint8_t alpha) {
  if (w <= 0 || h <= 0)
    return;
  const int r = std::max(0, std::min(radius, std::min(w, h) / 2));
  if (r <= 0) {
    this->fill_rect(x, y, w, h, color, alpha);
    return;
  }
  const int mid_w = w - 2 * r;
  const int mid_h = h - 2 * r;
  if (mid_w > 0)
    this->fill_rect(x + r, y, mid_w, h, color, alpha);
  if (mid_h > 0) {
    this->fill_rect(x, y + r, r, mid_h, color, alpha);
    this->fill_rect(x + w - r, y + r, r, mid_h, color, alpha);
  }

  const float cx = x + w * 0.5f;
  const float cy = y + h * 0.5f;
  const float hw = w * 0.5f;
  const float hh = h * 0.5f;
  const float rf = static_cast<float>(r);
  const auto paint_corner = [&](int x0, int y0, int x1, int y1) {
    for (int yy = y0; yy < y1; yy++) {
      for (int xx = x0; xx < x1; xx++) {
        const float sdf = sdf_round_rect(xx + 0.5f, yy + 0.5f, cx, cy, hw, hh, rf);
        if (!antialias) {
          if (sdf <= 0.0f)
            this->blend_pixel(xx, yy, color, alpha);
        } else {
          const float cov = clampf(0.5f - sdf, 0.0f, 1.0f);
          if (cov > 0.0f)
            this->blend_pixel(xx, yy, color, static_cast<uint8_t>(alpha * cov + 0.5f));
        }
      }
    }
  };
  paint_corner(x, y, x + r, y + r);
  paint_corner(x + w - r, y, x + w, y + r);
  paint_corner(x, y + h - r, x + r, y + h);
  paint_corner(x + w - r, y + h - r, x + w, y + h);
}

void DrawContext::fill_ellipse(int x, int y, int w, int h, bool antialias, Color color, uint8_t alpha) {
  if (w <= 0 || h <= 0)
    return;
  const float cx = x + w * 0.5f;
  const float cy = y + h * 0.5f;
  const float rx = std::max(0.5f, w * 0.5f);
  const float ry = std::max(0.5f, h * 0.5f);
  if (!antialias) {
    for (int yy = y; yy < y + h; yy++) {
      const float ny = (yy + 0.5f - cy) / ry;
      const float t = 1.0f - ny * ny;
      if (t < 0.0f)
        continue;
      const float half = sqrtf(t) * rx;
      const int x0 = static_cast<int>(ceilf(cx - half - 0.5f));
      const int x1 = static_cast<int>(floorf(cx + half - 0.5f));
      if (x1 >= x0)
        this->fill_rect(x0, yy, x1 - x0 + 1, 1, color, alpha);
    }
    return;
  }
  const float scale = std::min(rx, ry);
  for (int yy = y; yy < y + h; yy++) {
    for (int xx = x; xx < x + w; xx++) {
      const float nx = (xx + 0.5f - cx) / rx;
      const float ny = (yy + 0.5f - cy) / ry;
      const float d = sqrtf(nx * nx + ny * ny);
      const float sdf = (d - 1.0f) * scale;
      const float cov = clampf(0.5f - sdf, 0.0f, 1.0f);
      if (cov > 0.0f)
        this->blend_pixel(xx, yy, color, static_cast<uint8_t>(alpha * cov + 0.5f));
    }
  }
}

void DrawContext::hline(int x, int y, int w, Color color, uint8_t alpha) {
  this->fill_rect(x, y, w, 1, color, alpha);
}

void DrawContext::vline(int x, int y, int h, Color color, uint8_t alpha) {
  this->fill_rect(x, y, 1, h, color, alpha);
}

void DrawContext::line(int x0, int y0, int x1, int y1, Color color, uint8_t alpha) {
  const int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
  const int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
  int err = dx + dy;
  while (true) {
    this->blend_pixel(x0, y0, color, alpha);
    if (x0 == x1 && y0 == y1)
      break;
    const int e2 = 2 * err;
    if (e2 >= dy) {
      err += dy;
      x0 += sx;
    }
    if (e2 <= dx) {
      err += dx;
      y0 += sy;
    }
  }
}

static float orient2d(float ax, float ay, float bx, float by, float cx, float cy) {
  return (bx - ax) * (cy - ay) - (by - ay) * (cx - ax);
}

void DrawContext::fill_triangle(int x0, int y0, int x1, int y1, int x2, int y2, bool antialias, Color color,
                                uint8_t alpha) {
  const float area = orient2d(static_cast<float>(x0), static_cast<float>(y0), static_cast<float>(x1),
                              static_cast<float>(y1), static_cast<float>(x2), static_cast<float>(y2));
  if (fabsf(area) < 0.25f) {
    this->line(x0, y0, x1, y1, color, alpha);
    this->line(x1, y1, x2, y2, color, alpha);
    this->line(x2, y2, x0, y0, color, alpha);
    return;
  }
  const int minx = std::min(std::min(x0, x1), x2);
  const int miny = std::min(std::min(y0, y1), y2);
  const int maxx = std::max(std::max(x0, x1), x2);
  const int maxy = std::max(std::max(y0, y1), y2);
  const float s = area >= 0.0f ? 1.0f : -1.0f;
  const float e01 = hypotf(static_cast<float>(x1 - x0), static_cast<float>(y1 - y0));
  const float e12 = hypotf(static_cast<float>(x2 - x1), static_cast<float>(y2 - y1));
  const float e20 = hypotf(static_cast<float>(x0 - x2), static_cast<float>(y0 - y2));
  for (int y = miny; y <= maxy; y++) {
    for (int x = minx; x <= maxx; x++) {
      const float px = x + 0.5f;
      const float py = y + 0.5f;
      const float w0 = s * orient2d(static_cast<float>(x1), static_cast<float>(y1), static_cast<float>(x2),
                                    static_cast<float>(y2), px, py);
      const float w1 = s * orient2d(static_cast<float>(x2), static_cast<float>(y2), static_cast<float>(x0),
                                    static_cast<float>(y0), px, py);
      const float w2 = s * orient2d(static_cast<float>(x0), static_cast<float>(y0), static_cast<float>(x1),
                                    static_cast<float>(y1), px, py);
      if (!antialias) {
        if (w0 >= 0.0f && w1 >= 0.0f && w2 >= 0.0f)
          this->blend_pixel(x, y, color, alpha);
      } else {
        const float d0 = e12 > 0.0f ? w0 / e12 : w0;
        const float d1 = e20 > 0.0f ? w1 / e20 : w1;
        const float d2 = e01 > 0.0f ? w2 / e01 : w2;
        const float cov = clampf(0.5f + std::min(std::min(d0, d1), d2), 0.0f, 1.0f);
        if (cov > 0.0f)
          this->blend_pixel(x, y, color, static_cast<uint8_t>(alpha * cov + 0.5f));
      }
    }
  }
}

void DrawContext::stroke_ellipse(int x, int y, int w, int h, int thickness, bool antialias, Color color, uint8_t alpha) {
  if (w <= 0 || h <= 0)
    return;
  const int t = std::max(1, thickness);
  if (t * 2 >= w || t * 2 >= h) {
    this->fill_ellipse(x, y, w, h, antialias, color, alpha);
    return;
  }
  const float cx = x + w * 0.5f;
  const float cy = y + h * 0.5f;
  const float rx = std::max(0.5f, w * 0.5f);
  const float ry = std::max(0.5f, h * 0.5f);
  const float inner_rx = std::max(0.25f, rx - t);
  const float inner_ry = std::max(0.25f, ry - t);
  const float scale = std::min(rx, ry);
  const float inner_scale = std::min(inner_rx, inner_ry);
  for (int yy = y; yy < y + h; yy++) {
    for (int xx = x; xx < x + w; xx++) {
      const float nx = (xx + 0.5f - cx) / rx;
      const float ny = (yy + 0.5f - cy) / ry;
      const float d = sqrtf(nx * nx + ny * ny);
      const float inx = (xx + 0.5f - cx) / inner_rx;
      const float iny = (yy + 0.5f - cy) / inner_ry;
      const float di = sqrtf(inx * inx + iny * iny);
      if (!antialias) {
        if (d <= 1.0f && di >= 1.0f)
          this->blend_pixel(xx, yy, color, alpha);
      } else {
        const float outer_cov = clampf(0.5f - (d - 1.0f) * scale, 0.0f, 1.0f);
        const float inner_hole = clampf(0.5f - (di - 1.0f) * inner_scale, 0.0f, 1.0f);
        const float cov = outer_cov * (1.0f - inner_hole);
        if (cov > 0.0f)
          this->blend_pixel(xx, yy, color, static_cast<uint8_t>(alpha * cov + 0.5f));
      }
    }
  }
}

static float dist_to_seg(float px, float py, float ax, float ay, float bx, float by) {
  const float vx = bx - ax;
  const float vy = by - ay;
  const float len2 = vx * vx + vy * vy;
  float u = 0.0f;
  if (len2 > 0.0f)
    u = clampf(((px - ax) * vx + (py - ay) * vy) / len2, 0.0f, 1.0f);
  const float dx = px - (ax + u * vx);
  const float dy = py - (ay + u * vy);
  return sqrtf(dx * dx + dy * dy);
}

void DrawContext::stroke_line(int x0, int y0, int x1, int y1, int thickness, Color color, uint8_t alpha) {
  const int t = std::max(1, thickness);
  if (t == 1) {
    this->line(x0, y0, x1, y1, color, alpha);
    return;
  }
  const float r = t * 0.5f;
  const int minx = std::min(x0, x1) - t;
  const int miny = std::min(y0, y1) - t;
  const int maxx = std::max(x0, x1) + t;
  const int maxy = std::max(y0, y1) + t;
  for (int y = miny; y <= maxy; y++) {
    for (int x = minx; x <= maxx; x++) {
      const float d = dist_to_seg(x + 0.5f, y + 0.5f, static_cast<float>(x0), static_cast<float>(y0),
                                  static_cast<float>(x1), static_cast<float>(y1));
      if (d <= r)
        this->blend_pixel(x, y, color, alpha);
    }
  }
}

void DrawContext::circle_outline(int cx, int cy, int r, Color color, uint8_t alpha) {
  int x = r;
  int y = 0;
  int err = 0;
  while (x >= y) {
    this->blend_pixel(cx + x, cy + y, color, alpha);
    this->blend_pixel(cx + y, cy + x, color, alpha);
    this->blend_pixel(cx - y, cy + x, color, alpha);
    this->blend_pixel(cx - x, cy + y, color, alpha);
    this->blend_pixel(cx - x, cy - y, color, alpha);
    this->blend_pixel(cx - y, cy - x, color, alpha);
    this->blend_pixel(cx + y, cy - x, color, alpha);
    this->blend_pixel(cx + x, cy - y, color, alpha);
    y++;
    if (err <= 0) {
      err += 2 * y + 1;
    }
    if (err > 0) {
      x--;
      err -= 2 * x + 1;
    }
  }
}

#ifdef USE_FONT
void DrawContext::measure_text(font::Font *font, const char *text, int *width, int *height) {
  if (font == nullptr || text == nullptr) {
    *width = 0;
    *height = 0;
    return;
  }
  int x_off = 0, baseline = 0;
  font->measure(text, width, &x_off, &baseline, height);
}

void DrawContext::draw_text(int x, int y, font::Font *font, Color color, uint8_t alpha, const char *text) {
  if (font == nullptr || text == nullptr)
    return;
  int x_at = x;
  const char *cursor = text;
  while (*cursor != '\0') {
    size_t length = 0;
    uint32_t code_point = 0;
    const uint8_t c1 = static_cast<uint8_t>(*cursor);
    if (c1 < 0x80) {
      code_point = c1;
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
    } else {
      length = 1;
    }
    cursor += length;
    const auto *glyph = font->find_glyph(code_point);
    if (glyph == nullptr) {
      if (!font->get_glyphs().empty())
        x_at += font->get_glyphs()[0].advance;
      continue;
    }
    const uint8_t *data = glyph->data;
    uint8_t bitmask = 0;
    uint8_t pixel_data = 0;
    const uint8_t bpp = font->get_bpp();
    const uint8_t bpp_max = static_cast<uint8_t>((1 << bpp) - 1);
    const int max_x = x_at + glyph->offset_x + glyph->width;
    const int max_y = y + glyph->offset_y + glyph->height;
    for (int gy = y + glyph->offset_y; gy != max_y; gy++) {
      for (int gx = x_at + glyph->offset_x; gx != max_x; gx++) {
        uint8_t pixel = 0;
        for (uint8_t bit_num = 0; bit_num != bpp; bit_num++) {
          if (bitmask == 0) {
            pixel_data = *data++;
            bitmask = 0x80;
          }
          pixel = static_cast<uint8_t>((pixel << 1) | ((pixel_data & bitmask) != 0));
          bitmask >>= 1;
        }
        if (pixel != 0) {
          const uint8_t a = static_cast<uint8_t>((uint16_t) alpha * pixel / bpp_max);
          this->blend_pixel(gx, gy, color, a);
        }
      }
    }
    x_at += glyph->advance;
  }
}

int SideIcons::extra_width() const {
#ifdef USE_FONT
  if (this->font_ == nullptr)
    return 0;
  const int slot = this->font_->get_height();
  int w = 0;
  if (!this->start_.empty())
    w += slot + this->gap_;
  if (!this->end_.empty())
    w += slot + this->gap_;
  return w;
#else
  return 0;
#endif
}

#ifdef USE_FONT
static int side_icon_y(const SideIcons &icons, int y, int content_h, int slot) {
  if (icons.align_ == IconAlign::TOP)
    return y;
  if (icons.align_ == IconAlign::BOTTOM)
    return y + std::max(0, content_h - slot);
  return y + std::max(0, (content_h - slot) / 2);
}
#endif

int SideIcons::draw_start(DrawContext &ctx, int x, int y, int content_h, uint8_t alpha, Color color) const {
#ifdef USE_FONT
  if (this->start_.empty() || this->font_ == nullptr)
    return x;
  const int slot = this->font_->get_height();
  ctx.draw_text(x, side_icon_y(*this, y, content_h, slot), this->font_, color, alpha, this->start_.c_str());
  return x + slot + this->gap_;
#else
  (void) ctx;
  (void) y;
  (void) content_h;
  (void) alpha;
  (void) color;
  return x;
#endif
}

void SideIcons::draw_end(DrawContext &ctx, int x, int y, int content_h, uint8_t alpha, Color color) const {
#ifdef USE_FONT
  if (this->end_.empty() || this->font_ == nullptr)
    return;
  const int slot = this->font_->get_height();
  ctx.draw_text(x + this->gap_, side_icon_y(*this, y, content_h, slot), this->font_, color, alpha, this->end_.c_str());
#else
  (void) ctx;
  (void) x;
  (void) y;
  (void) content_h;
  (void) alpha;
  (void) color;
#endif
}

#endif

#ifdef USE_IMAGE
void DrawContext::draw_image_region(image::Image *img, int dst_x, int dst_y, int src_x, int src_y, int w, int h,
                                    uint8_t alpha) {
  if (img == nullptr || w <= 0 || h <= 0 || alpha == 0)
    return;
  for (int yy = 0; yy < h; yy++) {
    for (int xx = 0; xx < w; xx++) {
      Color px = img->get_pixel(src_x + xx, src_y + yy, Color(255, 255, 255, 255), Color(0, 0, 0, 0));
      if (px.w == 0)
        continue;
      this->blend_pixel(dst_x + xx, dst_y + yy, px, alpha);
    }
  }
}
#endif

void Widget::layout(int origin_x, int origin_y, int avail_w, int avail_h) {
  this->box_x_ = origin_x + this->x_;
  this->box_y_ = origin_y + this->y_;
  const int iw = this->intrinsic_width();
  const int ih = this->intrinsic_height();
  this->box_w_ = this->width_ > 0 ? this->width_ : (iw > 0 ? iw : avail_w);
  this->box_h_ = this->height_ > 0 ? this->height_ : (ih > 0 ? ih : avail_h);
}

void Widget::bind(PixelLayout *host) {
  this->host_ = host;
  this->bind_visible_();
}

#ifdef USE_SENSOR
void Widget::add_visible_sensor(sensor::Sensor *sensor, uint8_t cmp, float value, uint8_t cmp2, float value2,
                                bool invert) {
  VisibleClause clause;
  clause.sensor = sensor;
  clause.cmp = cmp;
  clause.value = value;
  clause.cmp2 = cmp2;
  clause.value2 = value2;
  clause.invert = invert;
  this->visible_clauses_.push_back(clause);
}
#endif
#ifdef USE_TEXT_SENSOR
void Widget::add_visible_text(text_sensor::TextSensor *sensor, const std::string &state, bool invert) {
  VisibleClause clause;
  clause.text = sensor;
  clause.state = state;
  clause.invert = invert;
  this->visible_clauses_.push_back(clause);
}
#endif

bool VisibleClause::matches() const {
#ifdef USE_SENSOR
  auto visible_cmp = [](uint8_t cmp, float value, float threshold) -> bool {
    switch (cmp) {
      case 1:
        return value > threshold;
      case 2:
        return value >= threshold;
      case 3:
        return std::fabs(value - threshold) < 0.0001f;
      case 4:
        return std::fabs(value - threshold) >= 0.0001f;
      case 5:
        return value < threshold;
      case 6:
        return value <= threshold;
      default:
        return true;
    }
  };
  if (this->sensor != nullptr) {
    bool match = false;
    if (this->sensor->has_state() && !std::isnan(this->sensor->state)) {
      const float value = this->sensor->state;
      match = visible_cmp(this->cmp, value, this->value) && visible_cmp(this->cmp2, value, this->value2);
    }
    return this->invert ? !match : match;
  }
#endif
#ifdef USE_TEXT_SENSOR
  if (this->text != nullptr) {
    const bool match = this->text->has_state() && this->text->state == this->state;
    return this->invert ? !match : match;
  }
#endif
  return !this->invert;
}

void Widget::bind_visible_() {
  for (auto &clause : this->visible_clauses_) {
#ifdef USE_SENSOR
    if (clause.sensor != nullptr) {
      clause.sensor->add_on_state_callback([this](float) { this->recompute_visible_(); });
    }
#endif
#ifdef USE_TEXT_SENSOR
    if (clause.text != nullptr) {
      clause.text->add_on_state_callback([this](const std::string &) { this->recompute_visible_(); });
    }
#endif
  }
  this->recompute_visible_();
}

void Widget::recompute_visible_() {
  if (this->visible_clauses_.empty()) {
    if (!this->shown_) {
      this->shown_ = true;
      if (this->host_ != nullptr)
        this->host_->invalidate_layout();
    }
    return;
  }
  bool match = this->visible_match_all_;
  for (const auto &clause : this->visible_clauses_) {
    const bool ok = clause.matches();
    if (this->visible_match_all_)
      match = match && ok;
    else
      match = match || ok;
  }
  const bool next = this->visible_invert_ ? !match : match;
  if (next == this->shown_)
    return;
  this->shown_ = next;
  if (this->host_ != nullptr)
    this->host_->invalidate_layout();
}

bool Widget::prepare(uint32_t now_ms) { return this->animation_active_(now_ms); }

uint32_t Widget::tick_period_ms() const {
  if (this->anim_.type == AnimType::NONE)
    return 0;
  if (this->anim_.repeat < 0)
    return 33;
  if (!this->anim_.started)
    return 33;
  const uint32_t now = millis();
  if (now < this->anim_.start_ms + this->anim_.delay_ms)
    return 33;
  const uint32_t dur = this->anim_.duration_ms == 0 ? 1 : this->anim_.duration_ms;
  const int iter = static_cast<int>((now - this->anim_.start_ms - this->anim_.delay_ms) / dur);
  return iter <= this->anim_.repeat ? 33 : 0;
}

void Widget::mark_dirty() {
  if (this->host_ != nullptr)
    this->host_->request_redraw();
}

bool Widget::animation_active_(uint32_t now_ms) {
  if (this->anim_.type == AnimType::NONE)
    return false;
  this->ensure_anim_started_(now_ms);
  if (this->anim_.repeat < 0)
    return true;
  if (now_ms < this->anim_.start_ms + this->anim_.delay_ms)
    return true;
  const uint32_t dur = this->anim_.duration_ms == 0 ? 1 : this->anim_.duration_ms;
  const int iter = static_cast<int>((now_ms - this->anim_.start_ms - this->anim_.delay_ms) / dur);
  return iter <= this->anim_.repeat;
}

void Widget::ensure_anim_started_(uint32_t now_ms) {
  if (!this->anim_.started) {
    this->anim_.started = true;
    this->anim_.start_ms = now_ms;
  }
}

float Widget::anim_amount_(uint32_t now_ms) const {
  const uint32_t dur = this->anim_.duration_ms == 0 ? 1 : this->anim_.duration_ms;
  const auto apply_mode = [this](float p) {
    if (this->anim_.mode == AnimMode::OUT)
      return 1.0f - p;
    if (this->anim_.mode == AnimMode::IN_OUT)
      return p < 0.5f ? p * 2.0f : (1.0f - p) * 2.0f;
    return p;
  };
  if (now_ms < this->anim_.start_ms + this->anim_.delay_ms)
    return apply_mode(0.0f);
  const uint32_t t = now_ms - this->anim_.start_ms - this->anim_.delay_ms;
  const int iter = static_cast<int>(t / dur);
  if (this->anim_.repeat >= 0 && iter > this->anim_.repeat)
    return this->anim_.mode == AnimMode::IN ? 1.0f : 0.0f;
  const float p = static_cast<float>(t % dur) / static_cast<float>(dur);
  return apply_mode(p);
}

uint8_t Widget::anim_opacity_(uint32_t now_ms, uint8_t base) const {
  if (this->anim_.type == AnimType::NONE)
    return base;
  uint8_t a = base;
  switch (this->anim_.type) {
    case AnimType::FADE: {
      const float u = this->anim_amount_(now_ms);
      a = static_cast<uint8_t>(this->anim_.from_opacity +
                               (this->anim_.to_opacity - this->anim_.from_opacity) * u);
      break;
    }
    case AnimType::PULSE: {
      if (now_ms < this->anim_.start_ms + this->anim_.delay_ms)
        return static_cast<uint8_t>((uint16_t) base * this->anim_.from_opacity / 255);
      const uint32_t t = now_ms - this->anim_.start_ms - this->anim_.delay_ms;
      const uint32_t dur = this->anim_.duration_ms == 0 ? 1 : this->anim_.duration_ms;
      const int iter = static_cast<int>(t / dur);
      if (this->anim_.repeat >= 0 && iter > this->anim_.repeat)
        return static_cast<uint8_t>((uint16_t) base * this->anim_.to_opacity / 255);
      const float p = static_cast<float>(t % dur) / static_cast<float>(dur);
      a = static_cast<uint8_t>(this->anim_.from_opacity +
                               (this->anim_.to_opacity - this->anim_.from_opacity) * (0.5f + 0.5f * sinf(p * 6.283185f)));
      break;
    }
    case AnimType::BLINK: {
      if (now_ms < this->anim_.start_ms + this->anim_.delay_ms)
        return static_cast<uint8_t>((uint16_t) base * this->anim_.from_opacity / 255);
      const uint32_t t = now_ms - this->anim_.start_ms - this->anim_.delay_ms;
      const uint32_t dur = this->anim_.duration_ms == 0 ? 1 : this->anim_.duration_ms;
      const int iter = static_cast<int>(t / dur);
      if (this->anim_.repeat >= 0 && iter > this->anim_.repeat)
        return static_cast<uint8_t>((uint16_t) base * this->anim_.to_opacity / 255);
      const float p = static_cast<float>(t % dur) / static_cast<float>(dur);
      a = p < 0.5f ? this->anim_.to_opacity : this->anim_.from_opacity;
      break;
    }
    default:
      a = base;
      break;
  }
  return static_cast<uint8_t>((uint16_t) base * a / 255);
}

void Widget::anim_offset_(uint32_t now_ms, int *dx, int *dy) const {
  *dx = 0;
  *dy = 0;
  if (this->anim_.type != AnimType::SLIDE)
    return;
  const float u = this->anim_amount_(now_ms);
  *dx = static_cast<int>(this->anim_.dx * (1.0f - u));
  *dy = static_cast<int>(this->anim_.dy * (1.0f - u));
}

void ContainerWidget::bind(PixelLayout *host) {
  Widget::bind(host);
  for (auto *child : this->children_)
    child->bind(host);
}

bool ContainerWidget::prepare(uint32_t now_ms) {
  bool changed = Widget::prepare(now_ms);
  for (auto *child : this->children_)
    changed = child->prepare(now_ms) || changed;
  return changed;
}

uint32_t ContainerWidget::tick_period_ms() const {
  uint32_t period = Widget::tick_period_ms();
  for (auto *child : this->children_) {
    const uint32_t child_period = child->tick_period_ms();
    if (child_period == 0)
      continue;
    period = period == 0 ? child_period : std::min(period, child_period);
  }
  return period;
}

int StackWidget::intrinsic_width() const {
  if (this->width_ > 0)
    return this->width_;
  int w = 0;
  for (auto *child : this->children_)
    w = std::max(w, child->offset_x() + child->intrinsic_width());
  return w;
}

int StackWidget::intrinsic_height() const {
  if (this->height_ > 0)
    return this->height_;
  int h = 0;
  for (auto *child : this->children_)
    h = std::max(h, child->offset_y() + child->intrinsic_height());
  return h;
}

void StackWidget::layout(int origin_x, int origin_y, int avail_w, int avail_h) {
  Widget::layout(origin_x, origin_y, avail_w, avail_h);
  for (auto *child : this->children_)
    child->layout(this->box_x_, this->box_y_, this->box_w_, this->box_h_);
}

void StackWidget::draw(DrawContext &ctx, uint32_t now_ms) {
  this->ensure_anim_started_(now_ms);
  for (auto *child : this->children_)
    child->draw_if_shown(ctx, now_ms);
}

int RowWidget::intrinsic_width() const {
  if (this->width_ > 0)
    return this->width_;
  int w = 0;
  int shown = 0;
  for (auto *child : this->children_) {
    if (!child->is_shown())
      continue;
    if (shown)
      w += this->gap_;
    w += child->intrinsic_width();
    shown++;
  }
  return w;
}

int RowWidget::intrinsic_height() const {
  if (this->height_ > 0)
    return this->height_;
  int h = 0;
  for (auto *child : this->children_) {
    if (child->is_shown())
      h = std::max(h, child->intrinsic_height());
  }
  return h;
}

void RowWidget::layout(int origin_x, int origin_y, int avail_w, int avail_h) {
  Widget::layout(origin_x, origin_y, avail_w, avail_h);
  std::vector<Widget *> shown;
  for (auto *child : this->children_) {
    if (child->is_shown())
      shown.push_back(child);
  }
  int fixed = 0;
  int flex = 0;
  for (size_t i = 0; i < shown.size(); i++) {
    if (shown[i]->expanded())
      flex++;
    else
      fixed += shown[i]->intrinsic_width();
    if (i + 1 < shown.size())
      fixed += this->gap_;
  }
  const int leftover = std::max(0, this->box_w_ - fixed);
  const int flex_w = flex > 0 ? leftover / flex : 0;
  int extra = leftover;
  if (this->main_align_ == AlignMain::CENTER)
    extra = leftover / 2;
  else if (this->main_align_ == AlignMain::END)
    extra = leftover;
  else if (this->main_align_ == AlignMain::SPACE_BETWEEN)
    extra = 0;
  int x = this->box_x_ + (this->main_align_ == AlignMain::START || this->main_align_ == AlignMain::SPACE_BETWEEN ? 0
                                                                                                                : extra);
  const int gaps = static_cast<int>(shown.size()) > 1 ? static_cast<int>(shown.size()) - 1 : 0;
  const int space = (this->main_align_ == AlignMain::SPACE_BETWEEN && gaps > 0)
                        ? std::max(this->gap_, leftover / gaps)
                        : this->gap_;
  for (auto *child : shown) {
    const int cw = child->expanded() ? std::max(child->intrinsic_width(), flex_w) : child->intrinsic_width();
    const int ch = child->intrinsic_height() > 0 ? child->intrinsic_height() : this->box_h_;
    int y = this->box_y_;
    if (this->cross_align_ == AlignCross::CENTER)
      y += std::max(0, (this->box_h_ - ch) / 2);
    else if (this->cross_align_ == AlignCross::END)
      y += std::max(0, this->box_h_ - ch);
    child->layout(x, y, cw, this->box_h_);
    x += cw + space;
  }
}

void RowWidget::draw(DrawContext &ctx, uint32_t now_ms) {
  this->ensure_anim_started_(now_ms);
  for (auto *child : this->children_)
    child->draw_if_shown(ctx, now_ms);
}

int ColumnWidget::intrinsic_width() const {
  if (this->width_ > 0)
    return this->width_;
  int w = 0;
  for (auto *child : this->children_) {
    if (child->is_shown())
      w = std::max(w, child->intrinsic_width());
  }
  return w;
}

int ColumnWidget::intrinsic_height() const {
  if (this->height_ > 0)
    return this->height_;
  int h = 0;
  int shown = 0;
  for (auto *child : this->children_) {
    if (!child->is_shown())
      continue;
    if (shown)
      h += this->gap_;
    h += child->intrinsic_height();
    shown++;
  }
  return h;
}

void ColumnWidget::layout(int origin_x, int origin_y, int avail_w, int avail_h) {
  Widget::layout(origin_x, origin_y, avail_w, avail_h);
  int y = this->box_y_;
  for (auto *child : this->children_) {
    if (!child->is_shown())
      continue;
    const int ch = child->intrinsic_height();
    const int cw = child->intrinsic_width() > 0 ? child->intrinsic_width() : this->box_w_;
    int x = this->box_x_;
    if (this->cross_align_ == AlignCross::CENTER)
      x += std::max(0, (this->box_w_ - cw) / 2);
    else if (this->cross_align_ == AlignCross::END)
      x += std::max(0, this->box_w_ - cw);
    child->layout(x, y, this->box_w_, ch);
    y += ch + this->gap_;
  }
}

void ColumnWidget::draw(DrawContext &ctx, uint32_t now_ms) {
  this->ensure_anim_started_(now_ms);
  for (auto *child : this->children_)
    child->draw_if_shown(ctx, now_ms);
}

int BoxWidget::intrinsic_width() const {
  if (this->width_ > 0)
    return this->width_;
  return (this->child_ ? this->child_->intrinsic_width() : 0) + this->padding_ * 2;
}

int BoxWidget::intrinsic_height() const {
  if (this->height_ > 0)
    return this->height_;
  return (this->child_ ? this->child_->intrinsic_height() : 0) + this->padding_ * 2;
}

void BoxWidget::layout(int origin_x, int origin_y, int avail_w, int avail_h) {
  Widget::layout(origin_x, origin_y, avail_w, avail_h);
  if (this->child_ != nullptr) {
    this->child_->layout(this->box_x_ + this->padding_, this->box_y_ + this->padding_,
                         std::max(0, this->box_w_ - this->padding_ * 2),
                         std::max(0, this->box_h_ - this->padding_ * 2));
  }
}

void BoxWidget::draw(DrawContext &ctx, uint32_t now_ms) {
  this->ensure_anim_started_(now_ms);
  const uint8_t a = this->anim_opacity_(now_ms, this->opacity_);
  int dx = 0, dy = 0;
  this->anim_offset_(now_ms, &dx, &dy);
  if (this->has_fill_) {
    const int x = this->box_x_ + dx;
    const int y = this->box_y_ + dy;
    const int w = this->box_w_;
    const int h = this->box_h_;
    const int stroke = std::max(1, this->stroke_);
    switch (this->shape_) {
      case BoxShape::OVAL:
        ctx.fill_ellipse(x, y, w, h, this->antialias_, this->fill_, a);
        break;
      case BoxShape::ROUNDED:
      case BoxShape::PILL: {
        const int r = this->shape_ == BoxShape::PILL ? std::min(w, h) / 2 : this->radius_;
        ctx.fill_round_rect(x, y, w, h, r, this->antialias_, this->fill_, a);
        break;
      }
      case BoxShape::TRIANGLE: {
        const int xm = x + w / 2;
        const int ym = y + h / 2;
        const int xr = x + w;
        const int yb = y + h;
        int x0 = x, y0 = yb, x1 = xr, y1 = yb, x2 = xm, y2 = y;
        if (this->point_ == BoxPoint::DOWN) {
          x0 = x;
          y0 = y;
          x1 = xr;
          y1 = y;
          x2 = xm;
          y2 = yb;
        } else if (this->point_ == BoxPoint::LEFT) {
          x0 = xr;
          y0 = y;
          x1 = xr;
          y1 = yb;
          x2 = x;
          y2 = ym;
        } else if (this->point_ == BoxPoint::RIGHT) {
          x0 = x;
          y0 = y;
          x1 = x;
          y1 = yb;
          x2 = xr;
          y2 = ym;
        }
        ctx.fill_triangle(x0, y0, x1, y1, x2, y2, this->antialias_, this->fill_, a);
        break;
      }
      case BoxShape::DIAMOND: {
        const int xm = x + w / 2;
        const int ym = y + h / 2;
        const int xr = x + w;
        const int yb = y + h;
        ctx.fill_triangle(xm, y, xr, ym, xm, yb, this->antialias_, this->fill_, a);
        ctx.fill_triangle(xm, y, xm, yb, x, ym, this->antialias_, this->fill_, a);
        break;
      }
      case BoxShape::PLUS: {
        const int t = std::min(stroke, std::min(w, h));
        ctx.fill_rect(x, y + (h - t) / 2, w, t, this->fill_, a);
        ctx.fill_rect(x + (w - t) / 2, y, t, h, this->fill_, a);
        break;
      }
      case BoxShape::FRAME: {
        const int s = std::min(stroke, std::min(w, h));
        if (s * 2 >= w || s * 2 >= h) {
          ctx.fill_rect(x, y, w, h, this->fill_, a);
        } else {
          ctx.fill_rect(x, y, w, s, this->fill_, a);
          ctx.fill_rect(x, y + h - s, w, s, this->fill_, a);
          ctx.fill_rect(x, y + s, s, h - 2 * s, this->fill_, a);
          ctx.fill_rect(x + w - s, y + s, s, h - 2 * s, this->fill_, a);
        }
        break;
      }
      case BoxShape::RING:
        ctx.stroke_ellipse(x, y, w, h, stroke, this->antialias_, this->fill_, a);
        break;
      case BoxShape::LINE:
        ctx.stroke_line(x, y, x + std::max(0, w - 1), y + std::max(0, h - 1), stroke, this->fill_, a);
        break;
      case BoxShape::RECT:
      default:
        ctx.fill_rect(x, y, w, h, this->fill_, a);
        break;
    }
  }
  if (this->child_ != nullptr)
    this->child_->draw_if_shown(ctx, now_ms);
}

void BoxWidget::bind(PixelLayout *host) {
  Widget::bind(host);
  if (this->child_ != nullptr)
    this->child_->bind(host);
}

bool BoxWidget::prepare(uint32_t now_ms) {
  bool changed = Widget::prepare(now_ms);
  if (this->child_ != nullptr)
    changed = this->child_->prepare(now_ms) || changed;
  return changed;
}

uint32_t BoxWidget::tick_period_ms() const {
  uint32_t period = Widget::tick_period_ms();
  if (this->child_ == nullptr)
    return period;
  const uint32_t child_period = this->child_->tick_period_ms();
  if (child_period == 0)
    return period;
  return period == 0 ? child_period : std::min(period, child_period);
}

void PixelLayout::setup() {
  if (this->display_ == nullptr) {
    this->mark_failed();
    return;
  }
  const int w = this->display_->get_width();
  const int h = this->display_->get_height();
  this->buffer_ = new uint8_t[static_cast<size_t>(w) * static_cast<size_t>(h) * 3];  // NOLINT
  this->ctx_.init(this->buffer_, w, h);
  this->display_->set_auto_clear(false);
  this->display_->stop_poller();
  this->display_->set_writer([this](display::Display &it) { this->render_(it); });
  if (this->screens_.empty() && this->root_ != nullptr)
    this->add_screen(this->root_, 0);
  this->ensure_enabled_vectors_();
  this->load_prefs_();
  this->load_night_prefs_();
  for (Widget *screen : this->screens_) {
    screen->bind(this);
    screen->layout(0, 0, w, h);
  }
  this->laid_out_ = true;
  this->dirty_ = true;
  this->screen_started_ms_ = millis();
  if (!this->screen_seen_.empty() && this->screen_index_ < this->screen_seen_.size())
    this->screen_seen_[this->screen_index_] = 1;
  this->notify_playlist_();
  this->evaluate_night_schedule();
#ifdef USE_PIXEL_LAYOUT_SD_STORAGE
  this->sd_storage_.setup();
  if (this->sd_storage_.use_sd_layout()) {
    std::string sd_err;
    this->load_playlist_from_sd(this->sd_root_path_, &sd_err);
    if (!sd_err.empty())
      this->sd_storage_.set_status(sd_err);
  }
#endif
  this->set_timeout(0, [this]() { this->tick_(); });
}

void PixelLayout::dump_config() {
  ESP_LOGCONFIG(TAG, "Pixel layout");
  if (this->display_ != nullptr)
    ESP_LOGCONFIG(TAG, "  Display: %dx%d", this->display_->get_width(), this->display_->get_height());
  ESP_LOGCONFIG(TAG, "  Screens: %u", static_cast<unsigned>(this->screens_.size()));
  ESP_LOGCONFIG(TAG, "  Loop: %s", YESNO(this->screen_loop_));
  ESP_LOGCONFIG(TAG, "  Random: %s", YESNO(this->screen_random_));
  ESP_LOGCONFIG(TAG, "  Tick: %" PRIu32 "ms (adaptive)", this->period_ms_);
}

void PixelLayout::request_redraw() {
  this->dirty_ = true;
  this->set_timeout("pl_paint", 0, [this]() { this->tick_(); });
}

void PixelLayout::invalidate_layout() {
  this->laid_out_ = false;
  this->request_redraw();
}

void PixelLayout::set_root(Widget *root) {
  this->root_ = root;
  if (this->screens_.empty() && root != nullptr)
    this->add_screen(root, 0);
}

void PixelLayout::set_rotate_ms(uint32_t ms) {
  this->rotate_ms_ = ms == 0 ? 1 : ms;
  if (!this->prefs_loaded_)
    this->rotate_default_ms_ = this->rotate_ms_;
  this->save_prefs_();
  this->notify_playlist_();
}

void PixelLayout::set_transition(ScreenTransition type) {
  this->transition_ = type;
  if (!this->prefs_loaded_)
    this->transition_default_ = type;
  this->save_prefs_();
  this->notify_playlist_();
}

void PixelLayout::set_transition_ms(uint32_t ms) {
  this->transition_ms_ = ms;
  if (!this->prefs_loaded_)
    this->transition_default_ms_ = ms;
  this->save_prefs_();
  this->notify_playlist_();
}

void PixelLayout::set_screen_random(bool random) {
  this->screen_random_ = random;
  this->save_prefs_();
  this->notify_playlist_();
}

void PixelLayout::add_screen(Widget *root, uint32_t duration_ms) {
  this->add_screen(root, duration_ms, this->transition_, this->transition_ms_, "");
}

void PixelLayout::add_screen(Widget *root, uint32_t duration_ms, ScreenTransition transition, uint32_t transition_ms) {
  this->add_screen(root, duration_ms, transition, transition_ms, "");
}

void PixelLayout::add_screen(Widget *root, uint32_t duration_ms, ScreenTransition transition, uint32_t transition_ms,
                             const std::string &id) {
  if (root == nullptr)
    return;
  this->screens_.push_back(root);
  this->screen_duration_ms_.push_back(duration_ms);
  this->screen_transition_.push_back(transition);
  this->screen_transition_ms_.push_back(transition_ms);
  this->screen_seen_.push_back(0);
  this->screen_enabled_flags_.push_back(1);
  std::string sid = id;
  if (sid.empty())
    sid = "screen_" + std::to_string(this->screens_.size());
  this->screen_ids_.push_back(std::move(sid));
  if (this->root_ == nullptr)
    this->root_ = root;
}

void PixelLayout::ensure_enabled_vectors_() {
  while (this->screen_enabled_flags_.size() < this->screens_.size())
    this->screen_enabled_flags_.push_back(1);
  while (this->screen_ids_.size() < this->screens_.size())
    this->screen_ids_.push_back("screen_" + std::to_string(this->screen_ids_.size() + 1));
}

bool PixelLayout::screen_enabled_(size_t index) const {
  if (index >= this->screens_.size())
    return false;
  if (index >= this->screen_enabled_flags_.size())
    return true;
  return this->screen_enabled_flags_[index] != 0;
}

bool PixelLayout::is_screen_enabled(size_t index) const { return this->screen_enabled_(index); }

void PixelLayout::set_screen_enabled(size_t index, bool enabled) {
  this->ensure_enabled_vectors_();
  if (index >= this->screens_.size())
    return;
  this->screen_enabled_flags_[index] = enabled ? 1 : 0;
  this->save_prefs_();
  if (!enabled && !this->pinned_ && index == this->screen_index_)
    this->show_next_enabled();
  this->notify_playlist_();
}

const std::string &PixelLayout::screen_id(size_t index) const {
  static const std::string EMPTY;
  if (index >= this->screen_ids_.size())
    return EMPTY;
  return this->screen_ids_[index];
}

std::string PixelLayout::current_screen_id() const { return this->screen_id(this->screen_index_); }

bool PixelLayout::show_screen(size_t index) {
  if (index >= this->screens_.size())
    return false;
  this->transitioning_ = false;
  this->screen_index_ = index;
  this->next_index_ = index;
  this->screen_started_ms_ = millis();
  if (index < this->screen_seen_.size())
    this->screen_seen_[index] = 1;
  this->request_redraw();
  this->save_prefs_();
  this->notify_playlist_();
  return true;
}

bool PixelLayout::show_screen(const std::string &id) {
  for (size_t i = 0; i < this->screen_ids_.size(); i++) {
    if (this->screen_ids_[i] == id)
      return this->show_screen(i);
  }
  return false;
}

bool PixelLayout::show_next_enabled() {
  if (this->pinned_)
    return false;
  const size_t next = this->choose_next_screen_();
  if (next == this->screen_index_)
    return false;
  return this->show_screen(next);
}

void PixelLayout::set_pinned(bool pinned) {
  this->pinned_ = pinned;
  if (!pinned)
    this->screen_started_ms_ = millis();
  this->save_prefs_();
  this->notify_playlist_();
}

void PixelLayout::set_rotate_override(bool on) {
  this->rotate_override_ = on;
  this->save_prefs_();
  this->notify_playlist_();
}

void PixelLayout::set_transition_override(bool on) {
  this->transition_override_ = on;
  this->save_prefs_();
  this->notify_playlist_();
}

void PixelLayout::notify_playlist_() {
  for (auto &cb : this->playlist_cbs_)
    cb();
}

struct PlaylistPrefs {
  uint8_t magic{0xA5};
  uint8_t pinned{0};
  uint8_t random{0};
  uint8_t rotate_override{0};
  uint8_t transition_override{0};
  uint8_t transition{0};
  uint16_t screen_index{0};
  uint32_t rotate_ms{8000};
  uint32_t transition_ms{400};
  uint32_t enabled_mask{0xFFFFFFFFu};
};

void PixelLayout::load_prefs_() {
  this->prefs_loaded_ = true;
  if (global_preferences == nullptr)
    return;
  auto pref = global_preferences->make_preference<PlaylistPrefs>(fnv1_hash("pixel_layout_playlist_v1"));
  PlaylistPrefs p{};
  if (!pref.load(&p) || p.magic != 0xA5)
    return;
  this->pinned_ = p.pinned != 0;
  this->screen_random_ = p.random != 0;
  this->rotate_override_ = p.rotate_override != 0;
  this->transition_override_ = p.transition_override != 0;
  if (p.rotate_ms > 0)
    this->rotate_ms_ = p.rotate_ms;
  this->transition_ms_ = p.transition_ms;
  this->transition_ = static_cast<ScreenTransition>(p.transition);
  this->ensure_enabled_vectors_();
  const size_t n = this->screens_.size();
  for (size_t i = 0; i < n && i < 32; i++)
    this->screen_enabled_flags_[i] = (p.enabled_mask & (1u << i)) ? 1 : 0;
  if (p.screen_index < n)
    this->screen_index_ = p.screen_index;
}

void PixelLayout::save_prefs_() {
  if (!this->prefs_loaded_ || global_preferences == nullptr)
    return;
  PlaylistPrefs p{};
  p.magic = 0xA5;
  p.pinned = this->pinned_ ? 1 : 0;
  p.random = this->screen_random_ ? 1 : 0;
  p.rotate_override = this->rotate_override_ ? 1 : 0;
  p.transition_override = this->transition_override_ ? 1 : 0;
  p.transition = static_cast<uint8_t>(this->transition_);
  p.screen_index = static_cast<uint16_t>(this->screen_index_);
  p.rotate_ms = this->rotate_ms_;
  p.transition_ms = this->transition_ms_;
  uint32_t mask = 0;
  const size_t n = std::min(this->screens_.size(), size_t{32});
  for (size_t i = 0; i < n; i++) {
    if (this->screen_enabled_(i))
      mask |= (1u << i);
  }
  p.enabled_mask = mask;
  auto pref = global_preferences->make_preference<PlaylistPrefs>(fnv1_hash("pixel_layout_playlist_v1"));
  pref.save(&p);
}

struct NightPrefs {
  uint8_t magic{0xA6};
  uint8_t enabled{1};
  uint8_t sleep_until_wake{0};
  uint8_t user_override{0};
  uint8_t off_hour{23};
  uint8_t off_minute{0};
  uint8_t on_hour{7};
  uint8_t on_minute{0};
};

void PixelLayout::load_night_prefs_() {
  if (!this->night_configured_ || global_preferences == nullptr)
    return;
  auto pref = global_preferences->make_preference<NightPrefs>(fnv1_hash("pixel_layout_night_v1"));
  NightPrefs p{};
  if (!pref.load(&p) || p.magic != 0xA6)
    return;
  this->night_enabled_ = p.enabled != 0;
  this->sleep_until_wake_ = p.sleep_until_wake != 0;
  this->user_override_ = p.user_override != 0;
  if (p.off_hour < 24)
    this->night_off_hour_ = p.off_hour;
  if (p.off_minute < 60)
    this->night_off_minute_ = p.off_minute;
  if (p.on_hour < 24)
    this->night_on_hour_ = p.on_hour;
  if (p.on_minute < 60)
    this->night_on_minute_ = p.on_minute;
}

void PixelLayout::save_night_prefs_() {
  if (!this->night_configured_ || !this->prefs_loaded_ || global_preferences == nullptr)
    return;
  NightPrefs p{};
  p.magic = 0xA6;
  p.enabled = this->night_enabled_ ? 1 : 0;
  p.sleep_until_wake = this->sleep_until_wake_ ? 1 : 0;
  p.user_override = this->user_override_ ? 1 : 0;
  p.off_hour = this->night_off_hour_;
  p.off_minute = this->night_off_minute_;
  p.on_hour = this->night_on_hour_;
  p.on_minute = this->night_on_minute_;
  auto pref = global_preferences->make_preference<NightPrefs>(fnv1_hash("pixel_layout_night_v1"));
  pref.save(&p);
}

void PixelLayout::set_night_schedule_enabled(bool on) {
  this->night_enabled_ = on;
  this->save_night_prefs_();
  this->evaluate_night_schedule();
  this->notify_playlist_();
}

void PixelLayout::set_night_off_hour(uint8_t hour) {
  this->night_off_hour_ = hour > 23 ? 23 : hour;
  this->save_night_prefs_();
  this->evaluate_night_schedule();
  this->notify_playlist_();
}

void PixelLayout::set_night_off_minute(uint8_t minute) {
  this->night_off_minute_ = minute > 59 ? 59 : minute;
  this->save_night_prefs_();
  this->evaluate_night_schedule();
  this->notify_playlist_();
}

void PixelLayout::set_night_on_hour(uint8_t hour) {
  this->night_on_hour_ = hour > 23 ? 23 : hour;
  this->save_night_prefs_();
  this->evaluate_night_schedule();
  this->notify_playlist_();
}

void PixelLayout::set_night_on_minute(uint8_t minute) {
  this->night_on_minute_ = minute > 59 ? 59 : minute;
  this->save_night_prefs_();
  this->evaluate_night_schedule();
  this->notify_playlist_();
}

void PixelLayout::sleep_until_wake() {
  if (!this->night_configured_)
    return;
  this->sleep_until_wake_ = true;
  this->user_override_ = false;
  this->save_night_prefs_();
  this->evaluate_night_schedule();
  this->notify_playlist_();
}

void PixelLayout::on_power_on() {
  if (!this->night_configured_)
    return;
  this->sleep_until_wake_ = false;
  this->user_override_ = true;
  this->save_night_prefs_();
  this->evaluate_night_schedule();
  this->notify_playlist_();
}

int PixelLayout::night_minutes_now_() const {
#ifdef USE_TIME
  if (this->night_time_ == nullptr)
    return -1;
  auto now = this->night_time_->now();
  if (!now.is_valid())
    return -1;
  return static_cast<int>(now.hour) * 60 + static_cast<int>(now.minute);
#else
  return -1;
#endif
}

bool PixelLayout::in_night_window_() const {
  const int now = this->night_minutes_now_();
  if (now < 0)
    return false;
  const int off = static_cast<int>(this->night_off_hour_) * 60 + static_cast<int>(this->night_off_minute_);
  const int on = static_cast<int>(this->night_on_hour_) * 60 + static_cast<int>(this->night_on_minute_);
  if (off == on)
    return false;
  if (off < on)
    return now >= off && now < on;
  return now >= off || now < on;
}

bool PixelLayout::should_blank_() const {
  if (!this->night_configured_)
    return false;
  if (this->sleep_until_wake_)
    return true;
  if (!this->night_enabled_)
    return false;
  if (this->user_override_)
    return false;
  return this->in_night_window_();
}

void PixelLayout::evaluate_night_schedule() {
  if (!this->night_configured_) {
    if (this->blanked_) {
      this->blanked_ = false;
      this->request_redraw();
    }
    return;
  }
  const int now_m = this->night_minutes_now_();
  if (now_m >= 0) {
    const bool in_win = this->in_night_window_();
    if (this->was_in_night_window_ && !in_win) {
      this->sleep_until_wake_ = false;
      this->user_override_ = false;
      this->save_night_prefs_();
    } else if (!this->was_in_night_window_ && in_win) {
      this->user_override_ = false;
      this->save_night_prefs_();
    }
    this->was_in_night_window_ = in_win;
  }
  const bool blank = this->should_blank_();
  if (blank != this->blanked_) {
    this->blanked_ = blank;
    this->request_redraw();
    this->notify_playlist_();
  }
}

ScreenTransition PixelLayout::transition_for_(size_t index) const {
  if (this->transition_override_)
    return this->transition_;
  if (index < this->screen_transition_.size())
    return this->screen_transition_[index];
  return this->transition_default_;
}

uint32_t PixelLayout::transition_ms_for_(size_t index) const {
  if (this->transition_override_)
    return this->transition_ms_;
  if (index < this->screen_transition_ms_.size())
    return this->screen_transition_ms_[index];
  return this->transition_default_ms_;
}

uint32_t PixelLayout::dwell_ms_for_(size_t index) const {
  if (this->rotate_override_)
    return this->rotate_ms_;
  uint32_t dwell = 0;
  if (index < this->screen_duration_ms_.size())
    dwell = this->screen_duration_ms_[index];
  if (dwell == 0)
    dwell = this->rotate_default_ms_;
  return dwell == 0 ? 1 : dwell;
}

Widget *PixelLayout::active_root_() const {
  if (!this->screens_.empty())
    return this->screens_[this->screen_index_];
  return this->root_;
}

size_t PixelLayout::next_screen_() const {
  if (this->screens_.size() < 2)
    return this->screen_index_;
  return this->next_index_;
}

size_t PixelLayout::choose_next_screen_() {
  const size_t n = this->screens_.size();
  if (n < 2)
    return this->screen_index_;

  auto collect = [&](bool require_unseen) {
    std::vector<size_t> candidates;
    candidates.reserve(n);
    for (size_t i = 0; i < n; i++) {
      if (i == this->screen_index_)
        continue;
      if (!this->screen_enabled_(i))
        continue;
      if (require_unseen && !this->screen_loop_ && i < this->screen_seen_.size() && this->screen_seen_[i])
        continue;
      candidates.push_back(i);
    }
    return candidates;
  };

  if (this->screen_random_) {
    auto candidates = collect(true);
    if (candidates.empty()) {
      if (!this->screen_loop_) {
        // Fall back to any other enabled screen, or stay.
        candidates = collect(false);
        if (candidates.empty())
          return this->screen_index_;
      } else {
        for (size_t i = 0; i < this->screen_seen_.size(); i++)
          this->screen_seen_[i] = 0;
        if (this->screen_index_ < this->screen_seen_.size())
          this->screen_seen_[this->screen_index_] = 1;
        candidates = collect(false);
      }
    }
    if (candidates.empty())
      return this->screen_index_;
    const size_t next = candidates[random_uint32() % candidates.size()];
    if (next < this->screen_seen_.size())
      this->screen_seen_[next] = 1;
    return next;
  }

  for (size_t step = 1; step <= n; step++) {
    const size_t i = (this->screen_index_ + step) % n;
    if (!this->screen_enabled_(i))
      continue;
    if (!this->screen_loop_ && i <= this->screen_index_ && step != 0)
      return this->screen_index_;
    return i;
  }
  return this->screen_index_;
}

void PixelLayout::advance_screens_(uint32_t now_ms) {
  if (this->blanked_)
    return;
  if (this->screens_.size() < 2)
    return;
  if (this->transitioning_) {
    if (now_ms - this->trans_started_ms_ >= this->trans_play_ms_) {
      this->transitioning_ = false;
      this->screen_index_ = this->next_index_;
      this->screen_started_ms_ = now_ms;
      this->dirty_ = true;
      this->save_prefs_();
      this->notify_playlist_();
    }
    return;
  }
  if (this->pinned_)
    return;
  const uint32_t dwell = this->dwell_ms_for_(this->screen_index_);
  if (now_ms - this->screen_started_ms_ < dwell)
    return;
  const size_t next = this->choose_next_screen_();
  if (next == this->screen_index_)
    return;
  this->next_index_ = next;
  this->trans_play_ = this->transition_for_(this->screen_index_);
  this->trans_play_ms_ = this->transition_ms_for_(this->screen_index_);
  if (this->trans_play_ != ScreenTransition::CUT && this->trans_play_ms_ > 0) {
    this->transitioning_ = true;
    this->trans_started_ms_ = now_ms;
  } else {
    this->screen_index_ = next;
    this->screen_started_ms_ = now_ms;
    this->save_prefs_();
    this->notify_playlist_();
  }
  this->dirty_ = true;
}

const char *screen_transition_name(ScreenTransition t) {
  switch (t) {
    case ScreenTransition::CUT:
      return "cut";
    case ScreenTransition::FADE:
      return "fade";
    case ScreenTransition::SLIDE_LEFT:
      return "slide_left";
    case ScreenTransition::SLIDE_RIGHT:
      return "slide_right";
    case ScreenTransition::SLIDE_UP:
      return "slide_up";
    case ScreenTransition::SLIDE_DOWN:
      return "slide_down";
    case ScreenTransition::WIPE_LEFT:
      return "wipe_left";
    case ScreenTransition::WIPE_RIGHT:
      return "wipe_right";
    case ScreenTransition::WIPE_UP:
      return "wipe_up";
    case ScreenTransition::WIPE_DOWN:
      return "wipe_down";
    case ScreenTransition::IRIS:
      return "iris";
    case ScreenTransition::DISSOLVE:
      return "dissolve";
    case ScreenTransition::BLINDS:
      return "blinds";
    default:
      return "fade";
  }
}

bool screen_transition_from_name(const char *name, ScreenTransition *out) {
  if (name == nullptr || out == nullptr)
    return false;
  const char *n = name;
  if (strcmp(n, "none") == 0 || strcmp(n, "cut") == 0) {
    *out = ScreenTransition::CUT;
    return true;
  }
  if (strcmp(n, "fade") == 0) {
    *out = ScreenTransition::FADE;
    return true;
  }
  if (strcmp(n, "slide") == 0 || strcmp(n, "slide_left") == 0) {
    *out = ScreenTransition::SLIDE_LEFT;
    return true;
  }
  if (strcmp(n, "slide_right") == 0) {
    *out = ScreenTransition::SLIDE_RIGHT;
    return true;
  }
  if (strcmp(n, "slide_up") == 0) {
    *out = ScreenTransition::SLIDE_UP;
    return true;
  }
  if (strcmp(n, "slide_down") == 0) {
    *out = ScreenTransition::SLIDE_DOWN;
    return true;
  }
  if (strcmp(n, "wipe") == 0 || strcmp(n, "wipe_left") == 0) {
    *out = ScreenTransition::WIPE_LEFT;
    return true;
  }
  if (strcmp(n, "wipe_right") == 0) {
    *out = ScreenTransition::WIPE_RIGHT;
    return true;
  }
  if (strcmp(n, "wipe_up") == 0) {
    *out = ScreenTransition::WIPE_UP;
    return true;
  }
  if (strcmp(n, "wipe_down") == 0) {
    *out = ScreenTransition::WIPE_DOWN;
    return true;
  }
  if (strcmp(n, "iris") == 0) {
    *out = ScreenTransition::IRIS;
    return true;
  }
  if (strcmp(n, "dissolve") == 0) {
    *out = ScreenTransition::DISSOLVE;
    return true;
  }
  if (strcmp(n, "blinds") == 0) {
    *out = ScreenTransition::BLINDS;
    return true;
  }
  return false;
}

void PixelLayout::tick_() {
  if (this->is_failed() || this->display_ == nullptr)
    return;
  const uint32_t now = millis();
  this->evaluate_night_schedule();
  this->advance_screens_(now);
  bool changed = this->dirty_;
  if (!this->blanked_) {
    Widget *cur = this->active_root_();
    if (cur != nullptr)
      changed = cur->prepare(now) || changed;
    if (this->transitioning_ && !this->screens_.empty()) {
      Widget *nxt = this->screens_[this->next_screen_()];
      if (nxt != nullptr)
        changed = nxt->prepare(now) || changed;
    }
  }
  if (changed) {
    this->dirty_ = true;
    this->display_->update();
    this->dirty_ = false;
  }
  this->reschedule_();
}

void PixelLayout::reschedule_() {
  const uint32_t period = this->tick_period_ms_();
  this->period_ms_ = period;
  if (period == 0)
    return;
  this->set_interval("pl_tick", period, [this]() { this->tick_(); });
}

uint32_t PixelLayout::tick_period_ms_() const {
  uint32_t period = 0;
  Widget *cur = this->active_root_();
  if (cur != nullptr)
    period = cur->tick_period_ms();
  if (this->screens_.size() > 1) {
    const uint32_t rot = this->transitioning_ ? 33 : 100;
    period = period == 0 ? rot : std::min(period, rot);
  }
  return period == 0 ? 1000 : period;
}

void PixelLayout::host_init(int width, int height) {
  if (width < 1)
    width = 1;
  if (height < 1)
    height = 1;
  delete[] this->buffer_;
  this->buffer_ = new uint8_t[static_cast<size_t>(width) * static_cast<size_t>(height) * 3];  // NOLINT
  this->ctx_.init(this->buffer_, width, height);
  if (this->screens_.empty() && this->root_ != nullptr)
    this->add_screen(this->root_, 0);
  for (Widget *screen : this->screens_) {
    screen->bind(this);
    screen->layout(0, 0, width, height);
  }
  this->ensure_enabled_vectors_();
  this->laid_out_ = true;
  this->dirty_ = true;
  this->screen_started_ms_ = millis();
  if (!this->screen_seen_.empty() && this->screen_index_ < this->screen_seen_.size())
    this->screen_seen_[this->screen_index_] = 1;
}

void PixelLayout::host_shutdown() {
  delete[] this->buffer_;
  this->buffer_ = nullptr;
  this->ctx_.init(nullptr, 0, 0);
}

void PixelLayout::host_set_play(size_t from, size_t to, bool transitioning, uint32_t trans_started_ms,
                                ScreenTransition kind, uint32_t trans_ms) {
  if (this->screens_.empty())
    return;
  if (from >= this->screens_.size())
    from = 0;
  if (to >= this->screens_.size())
    to = from;
  this->screen_index_ = from;
  this->next_index_ = to;
  this->transitioning_ = transitioning && from != to;
  this->trans_started_ms_ = trans_started_ms;
  this->trans_play_ = kind;
  this->trans_play_ms_ = trans_ms == 0 ? 1 : trans_ms;
}

void PixelLayout::host_paint() {
  const uint32_t now = millis();
  this->evaluate_night_schedule();
  if (!this->blanked_) {
    Widget *cur = this->active_root_();
    if (cur != nullptr)
      cur->prepare(now);
    if (this->transitioning_ && !this->screens_.empty()) {
      Widget *nxt = this->screens_[this->next_screen_()];
      if (nxt != nullptr)
        nxt->prepare(now);
    }
  }
  this->compose_(now);
}

void PixelLayout::compose_(uint32_t now) {
  this->ctx_.clear(this->background_);
  this->ctx_.set_alpha_scale(255);
  this->ctx_.set_origin(0, 0);
  if (this->blanked_)
    return;
  if (this->transitioning_ && this->screens_.size() > 1) {
    float t = static_cast<float>(now - this->trans_started_ms_) /
              static_cast<float>(this->trans_play_ms_ == 0 ? 1 : this->trans_play_ms_);
    if (t < 0)
      t = 0;
    if (t > 1)
      t = 1;
    Widget *prev = this->screens_[this->screen_index_];
    Widget *next = this->screens_[this->next_screen_()];
    const int w = this->ctx_.width();
    const int h = this->ctx_.height();
    int out_x = 0, out_y = 0, in_x = 0, in_y = 0;
    bool fade = false;
    bool masked = false;
    switch (this->trans_play_) {
      case ScreenTransition::SLIDE_LEFT:
        out_x = static_cast<int>(-t * w);
        in_x = static_cast<int>((1.0f - t) * w);
        break;
      case ScreenTransition::SLIDE_RIGHT:
        out_x = static_cast<int>(t * w);
        in_x = static_cast<int>((t - 1.0f) * w);
        break;
      case ScreenTransition::SLIDE_UP:
        out_y = static_cast<int>(-t * h);
        in_y = static_cast<int>((1.0f - t) * h);
        break;
      case ScreenTransition::SLIDE_DOWN:
        out_y = static_cast<int>(t * h);
        in_y = static_cast<int>((t - 1.0f) * h);
        break;
      case ScreenTransition::WIPE_LEFT:
      case ScreenTransition::WIPE_RIGHT:
      case ScreenTransition::WIPE_UP:
      case ScreenTransition::WIPE_DOWN:
      case ScreenTransition::IRIS:
      case ScreenTransition::DISSOLVE:
      case ScreenTransition::BLINDS:
        masked = true;
        break;
      case ScreenTransition::CUT:
        t = 1;
        break;
      case ScreenTransition::FADE:
      default:
        fade = true;
        break;
    }
    if (fade) {
      if (prev != nullptr) {
        this->ctx_.set_alpha_scale(static_cast<uint8_t>(255.0f * (1.0f - t)));
        prev->draw(this->ctx_, now);
      }
      if (next != nullptr) {
        this->ctx_.set_alpha_scale(static_cast<uint8_t>(255.0f * t));
        next->draw(this->ctx_, now);
      }
    } else if (masked) {
      if (prev != nullptr)
        prev->draw(this->ctx_, now);
      const int tw = static_cast<int>(t * static_cast<float>(w) + 0.5f);
      const int th = static_cast<int>(t * static_cast<float>(h) + 0.5f);
      switch (this->trans_play_) {
        case ScreenTransition::WIPE_LEFT:
          this->ctx_.set_clip(0, 0, tw, h);
          break;
        case ScreenTransition::WIPE_RIGHT:
          this->ctx_.set_clip(w - tw, 0, tw, h);
          break;
        case ScreenTransition::WIPE_UP:
          this->ctx_.set_clip(0, 0, w, th);
          break;
        case ScreenTransition::WIPE_DOWN:
          this->ctx_.set_clip(0, h - th, w, th);
          break;
        case ScreenTransition::IRIS: {
          const int ix = static_cast<int>((1.0f - t) * static_cast<float>(w) * 0.5f + 0.5f);
          const int iy = static_cast<int>((1.0f - t) * static_cast<float>(h) * 0.5f + 0.5f);
          this->ctx_.set_clip(ix, iy, w - 2 * ix, h - 2 * iy);
          break;
        }
        case ScreenTransition::DISSOLVE:
          this->ctx_.set_dissolve(static_cast<uint16_t>(t * 256.0f + 0.5f));
          break;
        case ScreenTransition::BLINDS:
          this->ctx_.set_blinds(static_cast<uint8_t>(t * 8.0f + 0.5f), 8, false);
          break;
        default:
          break;
      }
      if (next != nullptr)
        next->draw(this->ctx_, now);
      this->ctx_.reset_mask();
    } else {
      if (prev != nullptr) {
        this->ctx_.set_origin(out_x, out_y);
        prev->draw(this->ctx_, now);
      }
      if (next != nullptr) {
        this->ctx_.set_origin(in_x, in_y);
        next->draw(this->ctx_, now);
      }
    }
    this->ctx_.set_alpha_scale(255);
    this->ctx_.set_origin(0, 0);
  } else if (Widget *cur = this->active_root_()) {
    if (!this->laid_out_) {
      cur->layout(0, 0, this->ctx_.width(), this->ctx_.height());
      this->laid_out_ = true;
    }
    cur->draw(this->ctx_, now);
  }
}

void PixelLayout::render_(display::Display &it) {
  this->compose_(millis());
  this->ctx_.blit(it);
}

#ifdef USE_PIXEL_LAYOUT_SD_STORAGE
void PixelLayout::configure_sd_storage(const std::string &mount_path, const std::string &root_path, int clk_pin,
                                       int cmd_pin, int d0_pin, uint16_t upload_port) {
  this->sd_root_path_ = root_path;
  this->sd_storage_.configure(mount_path, root_path, clk_pin, cmd_pin, d0_pin, upload_port);
  this->sd_storage_.set_layout(this);
}

#ifdef USE_SENSOR
void PixelLayout::register_sensor_lookup(const std::string &id, sensor::Sensor *sensor) {
  if (id.empty() || sensor == nullptr)
    return;
  this->sensor_lookup_.emplace_back(id, sensor);
}

sensor::Sensor *PixelLayout::lookup_sensor(const char *id) const {
  if (id == nullptr)
    return nullptr;
  for (const auto &entry : this->sensor_lookup_) {
    if (entry.first == id)
      return entry.second;
  }
  return nullptr;
}
#endif

#ifdef USE_TEXT_SENSOR
void PixelLayout::register_text_sensor_lookup(const std::string &id, text_sensor::TextSensor *sensor) {
  if (id.empty() || sensor == nullptr)
    return;
  this->text_sensor_lookup_.emplace_back(id, sensor);
}

text_sensor::TextSensor *PixelLayout::lookup_text_sensor(const char *id) const {
  if (id == nullptr)
    return nullptr;
  for (const auto &entry : this->text_sensor_lookup_) {
    if (entry.first == id)
      return entry.second;
  }
  return nullptr;
}
#endif

#ifdef USE_TIME
void PixelLayout::register_time_lookup(const std::string &id, time::RealTimeClock *rtc) {
  if (id.empty() || rtc == nullptr)
    return;
  this->time_lookup_.emplace_back(id, rtc);
}

time::RealTimeClock *PixelLayout::lookup_time(const char *id) const {
  if (id == nullptr)
    return nullptr;
  for (const auto &entry : this->time_lookup_) {
    if (entry.first == id)
      return entry.second;
  }
  return nullptr;
}
#endif

#ifdef USE_FONT
void PixelLayout::register_font_lookup(const std::string &id, font::Font *font) {
  if (id.empty() || font == nullptr)
    return;
  this->font_lookup_.emplace_back(id, font);
}

font::Font *PixelLayout::lookup_font(const char *id) const {
  if (id == nullptr)
    return this->font_;
  for (const auto &entry : this->font_lookup_) {
    if (entry.first == id)
      return entry.second;
  }
  return this->font_;
}
#endif

void PixelLayout::backup_progmem_playlist_() {
  if (this->progmem_playlist_saved_)
    return;
  this->progmem_screens_ = this->screens_;
  this->progmem_duration_ms_ = this->screen_duration_ms_;
  this->progmem_transition_ = this->screen_transition_;
  this->progmem_transition_ms_ = this->screen_transition_ms_;
  this->progmem_screen_ids_ = this->screen_ids_;
  this->progmem_enabled_flags_ = this->screen_enabled_flags_;
  this->progmem_seen_ = this->screen_seen_;
  this->progmem_playlist_saved_ = true;
}

void PixelLayout::clear_active_playlist_() {
  this->sd_owned_widgets_.clear();
  this->sd_owned_images_.clear();
  this->screens_.clear();
  this->screen_duration_ms_.clear();
  this->screen_transition_.clear();
  this->screen_transition_ms_.clear();
  this->screen_ids_.clear();
  this->screen_enabled_flags_.clear();
  this->screen_seen_.clear();
  this->root_ = nullptr;
  this->screen_index_ = 0;
  this->next_index_ = 0;
  this->laid_out_ = false;
}

bool PixelLayout::apply_sd_playlist(const std::vector<SdScreenSpec> &specs, std::vector<std::unique_ptr<Widget>> owned,
                                    std::vector<SdOwnedImage> images, std::string *err) {
  if (specs.empty()) {
    if (err)
      *err = "empty sd playlist";
    return false;
  }
  this->backup_progmem_playlist_();
  this->clear_active_playlist_();
  this->sd_owned_widgets_ = std::move(owned);
  this->sd_owned_images_ = std::move(images);
  for (const SdScreenSpec &spec : specs) {
    if (spec.root == nullptr)
      continue;
    this->add_screen(spec.root, spec.duration_ms, spec.transition, spec.transition_ms, spec.id);
  }
  if (this->screens_.empty()) {
    if (err)
      *err = "no sd screens applied";
    return false;
  }
  const int w = this->ctx_.width();
  const int h = this->ctx_.height();
  for (Widget *screen : this->screens_) {
    screen->bind(this);
    screen->layout(0, 0, w, h);
  }
  this->ensure_enabled_vectors_();
  this->load_prefs_();
  this->screen_index_ = std::min(this->screen_index_, this->screens_.size() - 1);
  this->next_index_ = this->screen_index_;
  this->screen_started_ms_ = millis();
  this->laid_out_ = true;
  this->dirty_ = true;
  this->notify_playlist_();
  return true;
}

void PixelLayout::restore_progmem_playlist(std::string *err) {
  if (!this->progmem_playlist_saved_) {
    if (err)
      *err = "no progmem playlist saved";
    return;
  }
  this->clear_active_playlist_();
  this->screens_ = this->progmem_screens_;
  this->screen_duration_ms_ = this->progmem_duration_ms_;
  this->screen_transition_ = this->progmem_transition_;
  this->screen_transition_ms_ = this->progmem_transition_ms_;
  this->screen_ids_ = this->progmem_screen_ids_;
  this->screen_enabled_flags_ = this->progmem_enabled_flags_;
  this->screen_seen_ = this->progmem_seen_;
  this->root_ = this->screens_.empty() ? nullptr : this->screens_[0];
  this->screen_index_ = std::min(this->screen_index_, this->screens_.empty() ? 0 : this->screens_.size() - 1);
  this->laid_out_ = false;
  this->dirty_ = true;
  this->notify_playlist_();
  if (err)
    *err = "restored progmem playlist";
}

bool PixelLayout::load_playlist_from_sd(const std::string &root, std::string *err) {
  if (!this->sd_storage_.use_sd_layout()) {
    this->restore_progmem_playlist(err);
    return true;
  }
  SdPlaylistLoader loader(this);
  return loader.load(root, err);
}

void PixelLayout::reload_from_sd() {
  std::string err;
  if (this->load_playlist_from_sd(this->sd_root_path_, &err))
    this->sd_storage_.set_status(err.empty() ? "sd layout loaded" : err);
  else
    this->sd_storage_.set_status(err.empty() ? "sd reload failed" : err);
}
#endif

}  // namespace pixel_layout
}  // namespace esphome
