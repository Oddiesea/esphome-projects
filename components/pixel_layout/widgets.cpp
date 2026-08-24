#include "pixel_layout.h"

#include "esphome/core/hal.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace esphome {
namespace pixel_layout {

static Color outline_ink(Outline outline) {
  return outline == Outline::WHITE ? Color(255, 255, 255) : Color(0, 0, 0);
}

template <typename Fn>
static void paint_with_outline(Outline outline, bool allow, Fn &&paint) {
  if (allow && outline != Outline::NONE) {
    const Color ink = outline_ink(outline);
    for (int oy = -1; oy <= 1; oy++) {
      for (int ox = -1; ox <= 1; ox++) {
        if (ox == 0 && oy == 0)
          continue;
        paint(ox, oy, &ink);
      }
    }
  }
  paint(0, 0, nullptr);
}

static int align_in_box(IconAlign align, int box_start, int box_size, int content_size) {
  if (box_size <= 0)
    return box_start;
  if (align == IconAlign::TOP)
    return box_start;
  if (align == IconAlign::BOTTOM)
    return box_start + std::max(0, box_size - content_size);
  return box_start + std::max(0, (box_size - content_size + 1) / 2);
}

static const ThemeMetrics METRICS_SEVEN{16, 28, 3, 2, 6, 2, false, 0, 0, false, 0, 0, 0, 0};
static const ThemeMetrics METRICS_ROUNDED{16, 32, 4, 2, 6, 2, true, 0, 0, false, 0, 0, 0, 0};
static const ThemeMetrics METRICS_BLOCK{6, 10, 2, 3, 2, 2, false, 2, 0, true, 0, 0, 0, 0};
static const ThemeMetrics METRICS_TINY{3, 5, 1, 1, 1, 1, false, 1, 0, true, 0, 0, 0, 0};
static const ThemeMetrics METRICS_FLAP{22, 36, 2, 3, 8, 2, false, 4, 0, true, 0, 0, 0, 0};
static const ThemeMetrics METRICS_PERSPECTIVE{24, 52, 4, 1, 5, 2, false, 0, 0, false, 7, 4, 2, 10};

const ThemeMetrics &theme_metrics(ClockTheme theme) {
  switch (theme) {
    case ClockTheme::ROUNDED:
      return METRICS_ROUNDED;
    case ClockTheme::BLOCK:
      return METRICS_BLOCK;
    case ClockTheme::TINY:
      return METRICS_TINY;
    case ClockTheme::SPLIT_FLAP:
      return METRICS_FLAP;
    case ClockTheme::PERSPECTIVE:
      return METRICS_PERSPECTIVE;
    case ClockTheme::SEVEN_SEGMENT:
    case ClockTheme::TYPEFACE:
    default:
      return METRICS_SEVEN;
  }
}

static uint8_t scale_px(int value, int num, int den, uint8_t min_value = 1) {
  const int out = (value * num + den / 2) / den;
  return static_cast<uint8_t>(std::max<int>(min_value, out));
}

ThemeMetrics clock_metrics(ClockTheme theme, ClockSize size) {
  ThemeMetrics m = theme_metrics(theme);
  if (theme == ClockTheme::TINY || theme == ClockTheme::TYPEFACE)
    return m;
  if (theme == ClockTheme::SPLIT_FLAP) {
    if (size == ClockSize::SM) {
      m.digit_w = 16;
      m.digit_h = 26;
      m.cell = 3;
      m.colon_w = 6;
      m.gap = 2;
    } else if (size == ClockSize::LG) {
      m.digit_w = 24;
      m.digit_h = 44;
      m.cell = 5;
      m.colon_w = 8;
      m.gap = 2;
    }
    return m;
  }
  if (theme == ClockTheme::PERSPECTIVE) {
    if (size == ClockSize::SM) {
      m.digit_w = 16;
      m.digit_h = 40;
      m.thickness = 3;
      m.thickness_top = 5;
      m.thickness_mid = 3;
      m.thickness_bot = 1;
      m.taper = 6;
      m.colon_w = 4;
      m.gap = 1;
    } else if (size == ClockSize::LG) {
      m.digit_w = 26;
      m.digit_h = 58;
      m.thickness = 4;
      m.thickness_top = 8;
      m.thickness_mid = 5;
      m.thickness_bot = 2;
      m.taper = 11;
      m.colon_w = 5;
      m.gap = 1;
    }
    return m;
  }
  if (theme == ClockTheme::BLOCK) {
    const uint8_t cell = size == ClockSize::SM ? 1 : size == ClockSize::LG ? 3 : 2;
    m.cell = cell;
    m.digit_w = static_cast<uint8_t>(3 * cell);
    m.digit_h = static_cast<uint8_t>(5 * cell);
    m.gap = static_cast<uint8_t>(cell + 1);
    m.colon_w = cell;
    m.colon_gap = cell;
    m.thickness = cell;
    return m;
  }
  int num = 2;
  int den = 2;
  if (size == ClockSize::SM) {
    num = 3;
    den = 4;
  } else if (size == ClockSize::LG) {
    num = 3;
    den = 2;
  }
  m.digit_w = scale_px(m.digit_w, num, den);
  m.digit_h = scale_px(m.digit_h, num, den);
  m.thickness = scale_px(m.thickness, num, den);
  m.gap = scale_px(m.gap, num, den);
  m.colon_w = scale_px(m.colon_w, num, den);
  m.colon_gap = scale_px(m.colon_gap, num, den);
  return m;
}

void measure_digital_clock(ClockTheme theme, ClockSize size, bool show_seconds, int *width, int *height,
                           bool show_colon) {
  const ThemeMetrics m = clock_metrics(theme, size);
  const int digits = show_seconds ? 6 : 4;
  const int colons = show_colon ? (show_seconds ? 2 : 1) : 0;
  const int gaps = std::max(0, digits + colons - 1);
  *width = digits * m.digit_w + colons * m.colon_w + gaps * m.gap;
  *height = m.digit_h;
}

static void tmpl_append(char *out, size_t *n, size_t cap, const char *s) {
  if (s == nullptr)
    return;
  while (*s != '\0' && *n + 1 < cap)
    out[(*n)++] = *s++;
}

static void tmpl_append_unescaped(char *out, size_t *n, size_t cap, const char *s, size_t len) {
  for (size_t i = 0; i < len && *n + 1 < cap; i++) {
    if (s[i] == '\\' && i + 1 < len && s[i + 1] == 'n') {
      out[(*n)++] = '\n';
      i++;
      continue;
    }
    out[(*n)++] = s[i];
  }
}

static void format_template_value(char *buf, size_t cap, const char *raw, float value) {
  int prec = 1;
  const char *p = raw != nullptr ? raw : "";
  if (p[0] == '%')
    p++;
  if (p[0] == '\0') {
    snprintf(buf, cap, "%.1f", value);
    return;
  }
  if (p[0] == 'f' || p[0] == 'F') {
    snprintf(buf, cap, "%.0f", value);
    return;
  }
  if (p[0] == '.' && std::isdigit(static_cast<unsigned char>(p[1]))) {
    prec = p[1] - '0';
    snprintf(buf, cap, "%.*f", prec, value);
    return;
  }
  snprintf(buf, cap, "%.1f", value);
}

static void render_text_template(char *out, size_t cap, const char *tmpl, float value, bool has_value, const char *state,
                                 const char *unit, const char *label) {
  size_t n = 0;
  if (tmpl == nullptr)
    tmpl = "";
  const bool braced = strchr(tmpl, '{') != nullptr;
  if (!braced) {
    if (has_value && strchr(tmpl, '%') != nullptr) {
      char buf[48];
      snprintf(buf, sizeof(buf), tmpl, value);
      tmpl_append(out, &n, cap, buf);
    } else if (tmpl[0] != '\0') {
      tmpl_append_unescaped(out, &n, cap, tmpl, strlen(tmpl));
    } else if (state != nullptr && state[0] != '\0') {
      tmpl_append(out, &n, cap, state);
    }
    out[n] = '\0';
    return;
  }
  for (const char *p = tmpl; *p != '\0' && n + 1 < cap;) {
    if (p[0] == '{' && p[1] == '{') {
      out[n++] = '{';
      p += 2;
      continue;
    }
    if (p[0] == '}' && p[1] == '}') {
      out[n++] = '}';
      p += 2;
      continue;
    }
    if (*p != '{') {
      if (p[0] == '\\' && p[1] == 'n') {
        out[n++] = '\n';
        p += 2;
        continue;
      }
      out[n++] = *p++;
      continue;
    }
    const char *end = strchr(p, '}');
    if (end == nullptr) {
      out[n++] = *p++;
      continue;
    }
    char token[32];
    size_t tlen = static_cast<size_t>(end - (p + 1));
    if (tlen >= sizeof(token))
      tlen = sizeof(token) - 1;
    memcpy(token, p + 1, tlen);
    token[tlen] = '\0';
    p = end + 1;
    char key[24];
    strncpy(key, token, sizeof(key) - 1);
    key[sizeof(key) - 1] = '\0';
    const char *raw_spec = "";
    char *colon = strchr(key, ':');
    if (colon != nullptr) {
      *colon = '\0';
      raw_spec = colon + 1;
    }
    if (strcmp(key, "value") == 0) {
      if (!has_value) {
        tmpl_append(out, &n, cap, "--");
      } else {
        char buf[24];
        format_template_value(buf, sizeof(buf), raw_spec, value);
        tmpl_append(out, &n, cap, buf);
      }
    } else if (strcmp(key, "state") == 0 || strcmp(key, "text") == 0) {
      tmpl_append(out, &n, cap, state);
    } else if (strcmp(key, "unit") == 0) {
      tmpl_append(out, &n, cap, unit);
    } else if (strcmp(key, "label") == 0 || strcmp(key, "name") == 0) {
      tmpl_append(out, &n, cap, label);
    } else {
      out[n++] = '{';
      tmpl_append(out, &n, cap, token);
      if (n + 1 < cap)
        out[n++] = '}';
    }
  }
  out[n] = '\0';
}

void TextWidget::render_() {
  float value = NAN;
  bool has_value = false;
#ifdef USE_SENSOR
  if (this->sensor_ != nullptr && this->sensor_->has_state()) {
    value = this->sensor_->get_state();
    has_value = !std::isnan(value);
  }
#endif
  const char *state = "";
#ifdef USE_TEXT_SENSOR
  if (!this->text_state_.empty())
    state = this->text_state_.c_str();
  else if (this->text_sensor_ != nullptr && this->text_sensor_->has_state())
    state = this->text_sensor_->state.c_str();
#endif
  char next[96];
  const char *tmpl = this->text_.c_str();
  if (tmpl[0] == '\0' && has_value)
    tmpl = "{value:.1f}";
  else if (tmpl[0] == '\0' && state[0] != '\0')
    tmpl = "{state}";
  render_text_template(next, sizeof(next), tmpl, value, has_value, state, this->unit_.c_str(),
                       this->caption_.c_str());
  if (strncmp(this->rendered_, next, sizeof(this->rendered_)) == 0)
    return;
  const bool had_text = this->rendered_[0] != '\0';
  strncpy(this->rendered_, next, sizeof(this->rendered_) - 1);
  this->rendered_[sizeof(this->rendered_) - 1] = '\0';
  if (had_text && this->anim_.type != AnimType::NONE)
    this->anim_.started = false;
  this->mark_dirty();
}

void TextWidget::on_sensor_state_(float value) {
  (void) value;
  this->render_();
}

void TextWidget::on_text_state_(const std::string &value) {
#ifdef USE_TEXT_SENSOR
  this->text_state_ = value;
#endif
  this->render_();
}

void TextWidget::bind(PixelLayout *host) {
  Widget::bind(host);
#ifdef USE_SENSOR
  if (this->sensor_ != nullptr) {
    this->sensor_->add_on_state_callback([this](float value) { this->on_sensor_state_(value); });
  }
#endif
#ifdef USE_TEXT_SENSOR
  if (this->text_sensor_ != nullptr) {
    this->text_sensor_->add_on_state_callback([this](const std::string &value) { this->on_text_state_(value); });
    if (this->text_sensor_->has_state())
      this->text_state_ = this->text_sensor_->state;
  }
#endif
  this->render_();
}

int TextWidget::intrinsic_width() const {
  if (this->width_ > 0)
    return this->width_;
  int text_w = 0;
#ifdef USE_FONT
  if (this->font_ != nullptr) {
    int w = 0, h = 0, x_off = 0, baseline = 0;
    const char *nl = strchr(this->rendered_, '\n');
    if (nl != nullptr) {
      char top[96];
      size_t tlen = static_cast<size_t>(nl - this->rendered_);
      if (tlen >= sizeof(top))
        tlen = sizeof(top) - 1;
      memcpy(top, this->rendered_, tlen);
      top[tlen] = '\0';
      int w2 = 0;
      this->font_->measure(top, &w, &x_off, &baseline, &h);
      this->font_->measure(nl + 1, &w2, &x_off, &baseline, &h);
      text_w = std::max(w, w2);
    } else {
      this->font_->measure(this->rendered_, &w, &x_off, &baseline, &h);
      text_w = w;
    }
  } else
#endif
  {
    text_w = static_cast<int>(strlen(this->rendered_) * 6);
  }
  return text_w + this->icons_.extra_width();
}

int TextWidget::intrinsic_height() const {
  if (this->height_ > 0)
    return this->height_;
#ifdef USE_FONT
  const int h = this->font_ != nullptr ? this->font_->get_height() : 12;
#else
  const int h = 12;
#endif
  const bool two = this->style_ == TextStyle::TWO_LINE || strchr(this->rendered_, '\n') != nullptr;
  if (two && (strchr(this->rendered_, '\n') != nullptr || !this->caption_.empty()))
    return std::max(h * 2 + 1, this->icons_.slot_height());
  return std::max(h, this->icons_.slot_height());
}

void TextWidget::draw(DrawContext &ctx, uint32_t now_ms) {
  this->ensure_anim_started_(now_ms);
  const uint8_t a = this->anim_opacity_(now_ms, this->opacity_);
  int dx = 0, dy = 0;
  this->anim_offset_(now_ms, &dx, &dy);
  const int x = this->box_x_ + dx;
  const int y = this->box_y_ + dy;
#ifdef USE_FONT
  const char *nl = strchr(this->rendered_, '\n');
  const int h = this->font_ != nullptr ? this->font_->get_height() : 12;
  const bool two = nl != nullptr || (this->style_ == TextStyle::TWO_LINE && !this->caption_.empty());
  const int content_h = two ? h * 2 + 1 : h;
  const int box_h = std::max({content_h, this->icons_.slot_height(), this->height_ > 0 ? this->height_ : 0});
  int text_w = 0;
  int ink_h = content_h;
  if (this->font_ != nullptr) {
    int w = 0, w2 = 0, x_off = 0, baseline = 0, mh = 0;
    if (nl != nullptr) {
      char top[96];
      size_t tlen = static_cast<size_t>(nl - this->rendered_);
      if (tlen >= sizeof(top))
        tlen = sizeof(top) - 1;
      memcpy(top, this->rendered_, tlen);
      top[tlen] = '\0';
      this->font_->measure(top, &w, &x_off, &baseline, &mh);
      int b2 = 0;
      this->font_->measure(nl + 1, &w2, &x_off, &b2, &mh);
      text_w = std::max(w, w2);
      if (baseline > 0)
        ink_h = std::min(content_h, baseline + 1 + (b2 > 0 ? b2 : h));
    } else if (two) {
      this->font_->measure(this->caption_.c_str(), &w, &x_off, &baseline, &mh);
      this->font_->measure(this->rendered_, &w2, &x_off, &baseline, &mh);
      text_w = std::max(w, w2);
    } else {
      this->font_->measure(this->rendered_, &text_w, &x_off, &baseline, &mh);
      if (baseline > 0)
        ink_h = std::min(content_h, baseline);
    }
  } else {
    text_w = static_cast<int>(strlen(this->rendered_) * 6);
  }
  const int text_y_off = align_in_box(this->text_align_, 0, box_h, ink_h);
  paint_with_outline(this->outline_, true, [&](int ox, int oy, const Color *ink) {
    const Color text_c = ink != nullptr ? *ink : this->color_;
    const Color icon_c = ink != nullptr ? *ink : this->icons_.color_;
    int tx = this->icons_.draw_start(ctx, x + ox, y + oy, box_h, a, icon_c);
    const int ty = y + oy + text_y_off;
    if (nl != nullptr) {
      char top[96];
      size_t tlen = static_cast<size_t>(nl - this->rendered_);
      if (tlen >= sizeof(top))
        tlen = sizeof(top) - 1;
      memcpy(top, this->rendered_, tlen);
      top[tlen] = '\0';
      ctx.draw_text(tx, ty, this->font_, text_c, a, top);
      ctx.draw_text(tx, ty + h + 1, this->font_, text_c, a, nl + 1);
    } else if (this->style_ == TextStyle::TWO_LINE && !this->caption_.empty()) {
      ctx.draw_text(tx, ty, this->font_, text_c, a, this->caption_.c_str());
      ctx.draw_text(tx, ty + h + 1, this->font_, text_c, a, this->rendered_);
    } else {
      ctx.draw_text(tx, ty, this->font_, text_c, a, this->rendered_);
    }
    this->icons_.draw_end(ctx, tx + text_w, y + oy, box_h, a, icon_c);
  });
#else
  (void) ctx;
  (void) a;
  (void) x;
  (void) y;
#endif
}

static const uint8_t SEGMENTS[10] = {
    0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F,
};
static const uint8_t FONT3X5[10][5] = {
    {0b111, 0b101, 0b101, 0b101, 0b111}, {0b010, 0b110, 0b010, 0b010, 0b111}, {0b111, 0b001, 0b111, 0b100, 0b111},
    {0b111, 0b001, 0b111, 0b001, 0b111}, {0b101, 0b101, 0b111, 0b001, 0b001}, {0b111, 0b100, 0b111, 0b001, 0b111},
    {0b111, 0b100, 0b111, 0b101, 0b111}, {0b111, 0b001, 0b001, 0b001, 0b001}, {0b111, 0b101, 0b111, 0b101, 0b111},
    {0b111, 0b101, 0b111, 0b001, 0b111},
};

static void draw_seg_h(DrawContext &ctx, int x, int y, int len, int thick, Color color, uint8_t alpha, bool rounded) {
  for (int t = 0; t < thick; t++) {
    int inset = 0;
    if (rounded)
      inset = (t == 0 || t == thick - 1) ? 1 : 0;
    ctx.hline(x + inset, y + t, std::max(1, len - inset * 2), color, alpha);
  }
}

static void draw_seg_v(DrawContext &ctx, int x, int y, int len, int thick, Color color, uint8_t alpha, bool rounded) {
  for (int t = 0; t < thick; t++) {
    int inset = 0;
    if (rounded)
      inset = (t == 0 || t == thick - 1) ? 1 : 0;
    ctx.vline(x + t, y + inset, std::max(1, len - inset * 2), color, alpha);
  }
}

static int perspective_inset(int dy, int h, int taper) {
  if (taper <= 0 || h <= 1 || dy <= 0)
    return 0;
  if (dy >= h - 1)
    return taper;
  // Nearest: keeps the diagonal as a crisp stair rather than a soft smear.
  return (taper * dy * 2 + (h - 1)) / (2 * (h - 1));
}

static int perspective_vthick(int dy, int h, int t_top, int t_bot) {
  if (h <= 1)
    return t_top;
  if (dy <= 0)
    return t_top;
  if (dy >= h - 1)
    return t_bot;
  return t_top + ((t_bot - t_top) * dy * 2 + (h - 1)) / (2 * (h - 1));
}

static void draw_perspective_h(DrawContext &ctx, int digit_x, int digit_y, int digit_w, int digit_h, int taper,
                               int t_v_top, int t_v_bot, int bar_y, int thick, Color color, uint8_t alpha) {
  for (int t = 0; t < thick; t++) {
    const int row = bar_y + t;
    const int dy = row - digit_y;
    if (dy < 0 || dy >= digit_h)
      continue;
    const int inset = perspective_inset(dy, digit_h, taper);
    const int vt = std::max(1, perspective_vthick(dy, digit_h, t_v_top, t_v_bot));
    const int x0 = digit_x + inset + vt;
    const int len = digit_w - 2 * inset - 2 * vt;
    if (len > 0)
      ctx.hline(x0, row, len, color, alpha);
  }
}

static void draw_perspective_v(DrawContext &ctx, int digit_x, int digit_y, int digit_w, int digit_h, int taper,
                               int t_v_top, int t_v_bot, int y0, int y1, bool right, Color color, uint8_t alpha) {
  if (y1 <= y0)
    return;
  for (int row = y0; row < y1; row++) {
    const int dy = row - digit_y;
    if (dy < 0 || dy >= digit_h)
      continue;
    const int inset = perspective_inset(dy, digit_h, taper);
    const int vt = std::max(1, perspective_vthick(dy, digit_h, t_v_top, t_v_bot));
    const int x = right ? (digit_x + digit_w - inset - vt) : (digit_x + inset);
    ctx.hline(x, row, vt, color, alpha);
  }
}

static void draw_perspective_mask(DrawContext &ctx, int x, int y, uint8_t mask, const ThemeMetrics &m, Color color,
                                  uint8_t alpha) {
  const int w = m.digit_w;
  const int h = m.digit_h;
  const int taper = m.taper;
  const int t_top = m.thickness_top ? m.thickness_top : m.thickness;
  const int t_mid = m.thickness_mid ? m.thickness_mid : m.thickness;
  const int t_bot = m.thickness_bot ? m.thickness_bot : m.thickness;
  const int t_v_top = std::max<int>(1, m.thickness);
  const int t_v_bot = std::max<int>(1, m.thickness_bot ? m.thickness_bot : 1);
  const int remain = std::max(0, h - t_top - t_mid - t_bot);
  const int v_up = remain / 2;
  const int y_a = y;
  const int y_up0 = y + t_top;
  const int y_g = y_up0 + v_up;
  const int y_lo0 = y_g + t_mid;
  const int y_d = y + h - t_bot;

  if (mask & 0x01)
    draw_perspective_h(ctx, x, y, w, h, taper, t_v_top, t_v_bot, y_a, t_top, color, alpha);
  if (mask & 0x02)
    draw_perspective_v(ctx, x, y, w, h, taper, t_v_top, t_v_bot, y_up0, y_g, true, color, alpha);
  if (mask & 0x04)
    draw_perspective_v(ctx, x, y, w, h, taper, t_v_top, t_v_bot, y_lo0, y_d, true, color, alpha);
  if (mask & 0x08)
    draw_perspective_h(ctx, x, y, w, h, taper, t_v_top, t_v_bot, y_d, t_bot, color, alpha);
  if (mask & 0x10)
    draw_perspective_v(ctx, x, y, w, h, taper, t_v_top, t_v_bot, y_lo0, y_d, false, color, alpha);
  if (mask & 0x20)
    draw_perspective_v(ctx, x, y, w, h, taper, t_v_top, t_v_bot, y_up0, y_g, false, color, alpha);
  if (mask & 0x40)
    draw_perspective_h(ctx, x, y, w, h, taper, t_v_top, t_v_bot, y_g, t_mid, color, alpha);
}

static void draw_seven_mask(DrawContext &ctx, int x, int y, uint8_t mask, const ThemeMetrics &m, Color color,
                            uint8_t alpha) {
  if (m.thickness_top) {
    draw_perspective_mask(ctx, x, y, mask, m, color, alpha);
    return;
  }
  const int t = m.thickness;
  const int w = m.digit_w;
  const int h = m.digit_h;
  const int mid = y + (h - t) / 2;
  const int vlen = (h - t * 3) / 2;
  if (mask & 0x01)
    draw_seg_h(ctx, x + t, y, w - t * 2, t, color, alpha, m.rounded);
  if (mask & 0x02)
    draw_seg_v(ctx, x + w - t, y + t, vlen, t, color, alpha, m.rounded);
  if (mask & 0x04)
    draw_seg_v(ctx, x + w - t, mid + t, vlen, t, color, alpha, m.rounded);
  if (mask & 0x10)
    draw_seg_v(ctx, x, mid + t, vlen, t, color, alpha, m.rounded);
  if (mask & 0x20)
    draw_seg_v(ctx, x, y + t, vlen, t, color, alpha, m.rounded);
  if (mask & 0x08)
    draw_seg_h(ctx, x + t, y + h - t, w - t * 2, t, color, alpha, m.rounded);
  if (mask & 0x40)
    draw_seg_h(ctx, x + t, mid, w - t * 2, t, color, alpha, m.rounded);
}

static void draw_seven_digit(DrawContext &ctx, int x, int y, int digit, const ThemeMetrics &m, Color color,
                             uint8_t alpha, uint8_t ghost_alpha, Color ghost_color) {
  if (ghost_alpha)
    draw_seven_mask(ctx, x, y, 0x7F, m, ghost_color, ghost_alpha);
  const uint8_t mask = (digit >= 0 && digit <= 9) ? SEGMENTS[digit] : 0x40;
  draw_seven_mask(ctx, x, y, mask, m, color, alpha);
}

static void draw_grid_digit(DrawContext &ctx, int x, int y, int digit, const ThemeMetrics &m, Color color,
                            uint8_t alpha) {
  if (digit < 0 || digit > 9)
    return;
  const int cell = m.cell == 0 ? 1 : m.cell;
  for (int row = 0; row < 5; row++) {
    for (int col = 0; col < 3; col++) {
      if ((FONT3X5[digit][row] >> (2 - col)) & 1) {
        ctx.fill_rect(x + col * (cell + m.cell_gap), y + row * (cell + m.cell_gap), cell, cell, color, alpha);
      }
    }
  }
}

static void draw_colon(DrawContext &ctx, int x, int y, const ThemeMetrics &m, Color color, uint8_t alpha) {
  const int size = std::max(1, static_cast<int>(m.colon_w / 2));
  ctx.fill_rect(x, y + m.digit_h / 3 - size / 2, size, size, color, alpha);
  ctx.fill_rect(x, y + 2 * m.digit_h / 3 - size / 2, size, size, color, alpha);
}

static const uint32_t SPLIT_FLAP_HOUR_MIN_MS = 900;
static const uint32_t SPLIT_FLAP_SEC_MS = 240;
static const uint32_t SPLIT_FLAP_HOUR_MIN_STAGGER_MS = 160;
static const uint32_t SPLIT_FLAP_SEC_STAGGER_MS = 40;

static uint32_t split_flap_duration_ms(int index) {
  return index >= 4 ? SPLIT_FLAP_SEC_MS : SPLIT_FLAP_HOUR_MIN_MS;
}

static uint32_t split_flap_stagger_ms(int index) {
  return index >= 4 ? SPLIT_FLAP_SEC_STAGGER_MS : SPLIT_FLAP_HOUR_MIN_STAGGER_MS;
}

static void pack_clock_digits(int hour, int minute, int second, bool valid, int *out) {
  if (!valid) {
    for (int i = 0; i < 6; i++)
      out[i] = -1;
    return;
  }
  out[0] = hour / 10;
  out[1] = hour % 10;
  out[2] = minute / 10;
  out[3] = minute % 10;
  out[4] = second / 10;
  out[5] = second % 10;
}

static void draw_flap_glyph_rows(DrawContext &ctx, int gx, int gy, int digit, int cell, Color color, uint8_t alpha,
                                 int split, float top_scale, float bot_scale) {
  if (digit < 0 || digit > 9 || cell < 1)
    return;
  const int glyph_h = 5 * cell;
  for (int row = 0; row < 5; row++) {
    for (int col = 0; col < 3; col++) {
      if (((FONT3X5[digit][row] >> (2 - col)) & 1) == 0)
        continue;
      const int sx = gx + col * cell;
      const int sy = gy + row * cell;
      for (int r = 0; r < cell; r++) {
        const int src = sy + r;
        int dst = src;
        if (src < split) {
          if (top_scale <= 0.02f)
            continue;
          dst = split - static_cast<int>((split - src) * top_scale + 0.5f);
        } else {
          if (bot_scale <= 0.02f)
            continue;
          dst = split + static_cast<int>((src - split) * bot_scale + 0.5f);
        }
        if (dst < gy || dst >= gy + glyph_h)
          continue;
        ctx.fill_rect(sx, dst, cell, 1, color, alpha);
      }
    }
  }
}

static Color scale_color(Color c, float f) {
  return Color(static_cast<uint8_t>(std::max(0, std::min(255, static_cast<int>(c.r * f + 0.5f)))),
               static_cast<uint8_t>(std::max(0, std::min(255, static_cast<int>(c.g * f + 0.5f)))),
               static_cast<uint8_t>(std::max(0, std::min(255, static_cast<int>(c.b * f + 0.5f)))));
}

static Color lift_color(Color c, float f) {
  return Color(static_cast<uint8_t>(std::min(255, static_cast<int>(c.r + (255 - c.r) * f + 0.5f))),
               static_cast<uint8_t>(std::min(255, static_cast<int>(c.g + (255 - c.g) * f + 0.5f))),
               static_cast<uint8_t>(std::min(255, static_cast<int>(c.b + (255 - c.b) * f + 0.5f))));
}

static void draw_split_flap_module(DrawContext &ctx, int x, int y, const ThemeMetrics &m, int from, int to, float t,
                                   Color color, uint8_t alpha, bool has_secondary, Color secondary) {
  const int w = m.digit_w;
  const int h = m.digit_h;
  const int cell = m.cell == 0 ? 4 : m.cell;
  const Color card = has_secondary ? secondary : Color(42, 36, 28);
  const Color card_bot = has_secondary ? scale_color(secondary, 0.67f) : Color(28, 24, 20);
  const Color rim = has_secondary ? lift_color(secondary, 0.35f) : Color(82, 72, 56);
  const Color split_c = has_secondary ? scale_color(secondary, 0.28f) : Color(10, 8, 6);
  const int mid = y + h / 2;
  ctx.fill_rect(x, y, w, mid - y, card, alpha);
  ctx.fill_rect(x, mid, w, y + h - mid, card_bot, alpha);
  ctx.fill_rect(x, y, w, 1, rim, alpha);
  ctx.fill_rect(x, y, 1, h, split_c, alpha);
  ctx.fill_rect(x + w - 1, y, 1, h, split_c, alpha);
  ctx.fill_rect(x, y + h - 1, w, 1, split_c, alpha);
  ctx.fill_rect(x + 1, mid - 1, std::max(1, w - 2), 2, split_c, alpha);

  const int gx = x + (w - 3 * cell) / 2;
  const int gy = y + (h - 5 * cell) / 2;
  int digit = to;
  float top_scale = 1.0f;
  const float bot_scale = 1.0f;
  if (t < 1.0f && from != to) {
    if (t < 0.5f) {
      digit = from;
      top_scale = 1.0f - t * 2.0f;
    } else {
      digit = to;
      top_scale = (t - 0.5f) * 2.0f;
    }
  }
  draw_flap_glyph_rows(ctx, gx, gy, digit, cell, color, alpha, mid, top_scale, bot_scale);
  if (t > 0.02f && t < 0.98f && from != to) {
    const int edge = mid - static_cast<int>((mid - gy) * top_scale);
    ctx.fill_rect(x + 1, edge, std::max(1, w - 2), 2, color, static_cast<uint8_t>((uint16_t) alpha * 180 / 255));
  }
}

uint32_t ClockWidget::tick_period_ms() const {
  const uint32_t anim = Widget::tick_period_ms();
  uint32_t period = 1000;
  if (this->blink_colon_ && this->show_colon_ && this->face_ == ClockFace::DIGITAL)
    period = 500;
  if (this->face_ == ClockFace::ANALOG && this->show_seconds_)
    period = 100;
  if (this->show_seconds_ && this->face_ == ClockFace::DIGITAL)
    period = std::min(period, static_cast<uint32_t>(1000));
  if (this->theme_ == ClockTheme::SPLIT_FLAP && this->flaps_active_(millis()))
    period = std::min(period, static_cast<uint32_t>(33));
  if (anim == 0)
    return period;
  return std::min(anim, period);
}

bool ClockWidget::refresh_time_(uint32_t now_ms) {
  int hour = 0, minute = 0, second = 0;
  bool valid = false;
#ifdef USE_TIME
  time::RealTimeClock *rtc = this->time_;
  if (rtc != nullptr) {
    auto now = rtc->now();
    valid = now.is_valid();
    hour = now.hour;
    minute = now.minute;
    second = now.second;
  }
  if (!valid && this->fallback_time_ != nullptr) {
    rtc = this->fallback_time_;
    auto now = rtc->now();
    valid = now.is_valid();
    hour = now.hour;
    minute = now.minute;
    second = now.second;
  }
#endif
  const bool colon_on = !this->show_colon_ || !this->blink_colon_ || ((now_ms / 500) % 2 == 0);
  bool changed = valid != this->time_valid_ || hour != this->hour_ || minute != this->minute_ ||
                 this->colon_on_ != colon_on;
  if (this->show_seconds_ || this->face_ == ClockFace::ANALOG)
    changed = changed || second != this->second_;
  if (!changed)
    return false;
  this->time_valid_ = valid;
  this->hour_ = hour;
  this->minute_ = minute;
  this->second_ = second;
  this->colon_on_ = colon_on;
  if (!valid) {
    snprintf(this->label_, sizeof(this->label_), this->show_seconds_ ? "--:--:--" : "--:--");
    return true;
  }
  if (this->theme_ == ClockTheme::TYPEFACE && !this->format_.empty()) {
#ifdef USE_TIME
    rtc->now().strftime(this->label_, sizeof(this->label_), this->format_.c_str());
#else
    this->label_[0] = '\0';
#endif
  } else if (!this->show_colon_) {
    if (this->show_seconds_)
      snprintf(this->label_, sizeof(this->label_), "%02d %02d %02d", hour, minute, second);
    else
      snprintf(this->label_, sizeof(this->label_), "%02d %02d", hour, minute);
  } else if (this->blink_colon_ && !colon_on) {
    if (this->show_seconds_)
      snprintf(this->label_, sizeof(this->label_), "%02d %02d %02d", hour, minute, second);
    else
      snprintf(this->label_, sizeof(this->label_), "%02d %02d", hour, minute);
  } else if (this->show_seconds_) {
    snprintf(this->label_, sizeof(this->label_), "%02d:%02d:%02d", hour, minute, second);
  } else {
    snprintf(this->label_, sizeof(this->label_), "%02d:%02d", hour, minute);
  }
  return true;
}

bool ClockWidget::prepare(uint32_t now_ms) {
  const int old_hour = this->hour_;
  const int old_minute = this->minute_;
  const int old_second = this->second_;
  const bool old_valid = this->time_valid_;
  bool changed = Widget::prepare(now_ms);
  changed = this->refresh_time_(now_ms) || changed;
  if (this->theme_ == ClockTheme::SPLIT_FLAP)
    changed = this->arm_flaps_(now_ms, old_valid, old_hour, old_minute, old_second) || changed;
  return changed;
}

bool ClockWidget::flaps_active_(uint32_t now_ms) const {
  if (this->theme_ != ClockTheme::SPLIT_FLAP)
    return false;
  for (int i = 0; i < 6; i++) {
    if (this->flap_on_[i] && now_ms < this->flap_until_ms_[i])
      return true;
  }
  return false;
}

bool ClockWidget::arm_flaps_(uint32_t now_ms, bool old_valid, int old_hour, int old_minute, int old_second) {
  int old_digits[6];
  int new_digits[6];
  pack_clock_digits(old_hour, old_minute, old_second, old_valid, old_digits);
  pack_clock_digits(this->hour_, this->minute_, this->second_, this->time_valid_, new_digits);
  const int count = this->show_seconds_ ? 6 : 4;
  bool digit_changed = false;
  for (int i = 0; i < count; i++) {
    if (old_digits[i] != new_digits[i]) {
      digit_changed = true;
      break;
    }
  }
  if (digit_changed) {
    for (int i = 0; i < count; i++) {
      this->flap_from_[i] = static_cast<int8_t>(old_digits[i]);
      this->flap_to_[i] = static_cast<int8_t>(new_digits[i]);
      if (old_digits[i] == new_digits[i]) {
        this->flap_on_[i] = false;
        continue;
      }
      const uint32_t delay = static_cast<uint32_t>(count - 1 - i) * split_flap_stagger_ms(i);
      this->flap_start_ms_[i] = now_ms + delay;
      this->flap_until_ms_[i] = this->flap_start_ms_[i] + split_flap_duration_ms(i);
      this->flap_on_[i] = true;
    }
  }
  bool active = false;
  for (int i = 0; i < 6; i++) {
    if (!this->flap_on_[i])
      continue;
    if (now_ms >= this->flap_until_ms_[i]) {
      this->flap_on_[i] = false;
      this->flap_from_[i] = this->flap_to_[i];
      continue;
    }
    active = true;
  }
  return digit_changed || active;
}

int ClockWidget::intrinsic_width() const {
  if (this->width_ > 0)
    return this->width_;
  if (this->face_ == ClockFace::ANALOG)
    return 40;
  int w = 0;
  if (this->theme_ == ClockTheme::TYPEFACE) {
#ifdef USE_FONT
    w = this->show_seconds_ ? 72 : 48;
#else
    w = 48;
#endif
  } else {
    int h = 0;
    measure_digital_clock(this->theme_, this->size_, this->show_seconds_, &w, &h, this->show_colon_);
  }
  if (this->face_ == ClockFace::DIGITAL)
    w += this->icons_.extra_width();
  return w;
}

int ClockWidget::intrinsic_height() const {
  if (this->height_ > 0)
    return this->height_;
  if (this->face_ == ClockFace::ANALOG)
    return this->width_ > 0 ? this->width_ : 40;
  if (this->theme_ == ClockTheme::TYPEFACE) {
#ifdef USE_FONT
    if (this->font_ != nullptr)
      return std::max(this->font_->get_height(), this->icons_.slot_height());
#endif
    return 16;
  }
  int w = 0, h = 0;
  measure_digital_clock(this->theme_, this->size_, this->show_seconds_, &w, &h, this->show_colon_);
  return std::max(h, this->icons_.slot_height());
}

void ClockWidget::draw_digital_(DrawContext &ctx, int x, int y, uint8_t alpha, Color color, uint32_t now_ms) {
  if (this->theme_ == ClockTheme::TYPEFACE) {
#ifdef USE_FONT
    ctx.draw_text(x, y, this->font_, color, alpha, this->label_);
#endif
    return;
  }

  const ThemeMetrics m = clock_metrics(this->theme_, this->size_);
  const bool halo = color.r == 0 && color.g == 0 && color.b == 0;
  const Color ghost_color = this->has_secondary_color_ ? this->secondary_color_ : color;
  const uint8_t ghost_alpha =
      (this->ghost_ && !m.grid)
          ? (this->has_secondary_color_
                 ? static_cast<uint8_t>((uint16_t) alpha * 90 / 255)
                 : (halo ? alpha : static_cast<uint8_t>((uint16_t) alpha * 48 / 255)))
          : 0;
  int digits[6] = {-1, -1, -1, -1, -1, -1};
  pack_clock_digits(this->hour_, this->minute_, this->second_, this->time_valid_, digits);
  const int count = this->show_seconds_ ? 6 : 4;
  int cx = x;
  for (int i = 0; i < count; i++) {
    if (i == 2 || i == 4) {
      if (this->show_colon_) {
        if (this->colon_on_)
          draw_colon(ctx, cx, y, m, color, alpha);
        else if (ghost_alpha)
          draw_colon(ctx, cx, y, m, ghost_color, ghost_alpha);
        cx += m.colon_w + m.gap;
      }
    }
    if (this->theme_ == ClockTheme::SPLIT_FLAP) {
      int from = this->flap_to_[i];
      int to = digits[i];
      float t = 1.0f;
      if (this->flap_on_[i]) {
        from = this->flap_from_[i];
        to = this->flap_to_[i];
        if (now_ms < this->flap_start_ms_[i])
          t = 0.0f;
        else if (now_ms >= this->flap_until_ms_[i])
          t = 1.0f;
        else {
          const uint32_t dur = this->flap_until_ms_[i] - this->flap_start_ms_[i];
          t = static_cast<float>(now_ms - this->flap_start_ms_[i]) / static_cast<float>(dur == 0 ? 1 : dur);
        }
      } else if (this->flap_to_[i] >= 0) {
        to = this->flap_to_[i];
        from = to;
      }
      draw_split_flap_module(ctx, cx, y, m, from, to, t, color, alpha, this->has_secondary_color_,
                             this->secondary_color_);
    } else if (m.grid) {
      draw_grid_digit(ctx, cx, y, digits[i], m, color, alpha);
    } else {
      draw_seven_digit(ctx, cx, y, digits[i], m, color, alpha, ghost_alpha, ghost_color);
    }
    cx += m.digit_w + m.gap;
  }
}

void ClockWidget::draw_analog_(DrawContext &ctx, int x, int y, uint8_t alpha, Color color) {
  const int size = std::min(this->box_w_, this->box_h_);
  const int cx = x + size / 2;
  const int cy = y + size / 2;
  const int r = size / 2 - 1;
  const ClockTheme style = this->theme_;
  const bool hub = style == ClockTheme::MINIMAL || style == ClockTheme::TICKS || style == ClockTheme::SQUARE;

  if (style == ClockTheme::SQUARE) {
    const int half = r;
    ctx.hline(cx - half, cy - half, half * 2 + 1, color, alpha);
    ctx.hline(cx - half, cy + half, half * 2 + 1, color, alpha);
    ctx.vline(cx - half, cy - half, half * 2 + 1, color, alpha);
    ctx.vline(cx + half, cy - half, half * 2 + 1, color, alpha);
    ctx.vline(cx, cy - half + 1, 3, color, alpha);
    ctx.vline(cx, cy + half - 3, 3, color, alpha);
    ctx.hline(cx - half + 1, cy, 3, color, alpha);
    ctx.hline(cx + half - 3, cy, 3, color, alpha);
  } else if (style == ClockTheme::RING) {
    ctx.circle_outline(cx, cy, r, color, alpha);
  }

  for (int i = 0; i < 12; i++) {
    const float a = (i * 30 - 90) * 0.0174532925f;
    const bool cardinal = (i % 3) == 0;
    if (style == ClockTheme::MINIMAL) {
      if (!cardinal)
        continue;
      const int x0 = cx + static_cast<int>(cosf(a) * (r - 1));
      const int y0 = cy + static_cast<int>(sinf(a) * (r - 1));
      const int x1 = cx + static_cast<int>(cosf(a) * (r - 4));
      const int y1 = cy + static_cast<int>(sinf(a) * (r - 4));
      ctx.line(x0, y0, x1, y1, color, alpha);
    } else if (style == ClockTheme::TICKS) {
      const int outer = r - 1;
      const int inner = r - (cardinal ? 5 : 3);
      const int x0 = cx + static_cast<int>(cosf(a) * outer);
      const int y0 = cy + static_cast<int>(sinf(a) * outer);
      const int x1 = cx + static_cast<int>(cosf(a) * inner);
      const int y1 = cy + static_cast<int>(sinf(a) * inner);
      ctx.line(x0, y0, x1, y1, color, alpha);
    } else if (style == ClockTheme::RING) {
      const int x0 = cx + static_cast<int>(cosf(a) * (r - 3));
      const int y0 = cy + static_cast<int>(sinf(a) * (r - 3));
      ctx.blend_pixel(x0, y0, color, alpha);
    }
  }

  const int hour = this->hour_;
  const int minute = this->minute_;
  const int second = this->second_;
  const float ha = ((hour % 12) * 30 + minute * 0.5f - 90) * 0.0174532925f;
  const float ma = (minute * 6 - 90) * 0.0174532925f;
  ctx.line(cx, cy, cx + static_cast<int>(cosf(ha) * (r * 0.5f)), cy + static_cast<int>(sinf(ha) * (r * 0.5f)), color,
           alpha);
  ctx.line(cx, cy, cx + static_cast<int>(cosf(ma) * (r * 0.8f)), cy + static_cast<int>(sinf(ma) * (r * 0.8f)), color,
           alpha);
  if (hub)
    ctx.blend_pixel(cx, cy, color, alpha);
  if (this->show_seconds_) {
    const bool halo = color.r == 0 && color.g == 0 && color.b == 0;
    const Color second_color =
        this->has_secondary_color_ ? this->secondary_color_ : (halo ? color : Color(255, 80, 80));
    const float sa = (second * 6 - 90) * 0.0174532925f;
    ctx.line(cx, cy, cx + static_cast<int>(cosf(sa) * (r * 0.9f)), cy + static_cast<int>(sinf(sa) * (r * 0.9f)),
             second_color, alpha);
  }
}

void ClockWidget::paint_clock_(DrawContext &ctx, int x, int y, uint8_t alpha, Color color, uint32_t now_ms) {
  if (this->face_ == ClockFace::ANALOG)
    this->draw_analog_(ctx, x, y, alpha, color);
  else
    this->draw_digital_(ctx, x, y, alpha, color, now_ms);
}

void ClockWidget::draw(DrawContext &ctx, uint32_t now_ms) {
  this->ensure_anim_started_(now_ms);
  const uint8_t a = this->anim_opacity_(now_ms, this->opacity_);
  int dx = 0, dy = 0;
  this->anim_offset_(now_ms, &dx, &dy);
  const int x = this->box_x_ + dx;
  const int y = this->box_y_ + dy;
  int content_w = this->box_w_;
  int content_h = this->box_h_;
  const bool digital = this->face_ == ClockFace::DIGITAL;
  if (digital) {
    if (this->theme_ == ClockTheme::TYPEFACE) {
#ifdef USE_FONT
      int x_off = 0, baseline = 0, mh = 0;
      this->font_->measure(this->label_, &content_w, &x_off, &baseline, &mh);
      content_h = mh > 0 ? mh : content_h;
      if (baseline > 0)
        content_h = std::min(content_h > 0 ? content_h : baseline, baseline);
#else
      content_w = this->show_seconds_ ? 72 : 48;
      content_h = 16;
#endif
    } else {
      measure_digital_clock(this->theme_, this->size_, this->show_seconds_, &content_w, &content_h, this->show_colon_);
    }
  }
  paint_with_outline(this->outline_, this->theme_ != ClockTheme::SPLIT_FLAP, [&](int ox, int oy, const Color *ink) {
    const Color clock_c = ink != nullptr ? *ink : this->color_;
    const Color icon_c = ink != nullptr ? *ink : this->icons_.color_;
    int cx = x + ox;
    const int cy = y + oy;
    const int box_h = std::max({content_h, this->icons_.slot_height(), this->height_ > 0 ? this->height_ : 0});
    const int ty = cy + align_in_box(this->text_align_, 0, box_h, content_h);
    if (digital)
      cx = this->icons_.draw_start(ctx, cx, cy, box_h, a, icon_c);
    this->paint_clock_(ctx, cx, ty, a, clock_c, now_ms);
    if (digital)
      this->icons_.draw_end(ctx, cx + content_w, cy, box_h, a, icon_c);
  });
}

static void ascii_upper(char *s) {
  for (; *s != '\0'; s++) {
    if (*s >= 'a' && *s <= 'z')
      *s = static_cast<char>(*s - 'a' + 'A');
  }
}

const char *weather_glyph(const char *condition) {
  if (condition == nullptr || condition[0] == '\0')
    return "\uf15c";  // cloudy
  char key[48];
  size_t j = 0;
  const char *in = condition;
  if (strncmp(in, "mdi:weather-", 12) == 0)
    in += 12;
  else if (strncmp(in, "weather-", 8) == 0)
    in += 8;
  for (; *in != '\0' && j + 1 < sizeof(key); in++) {
    char c = *in;
    if (c >= 'A' && c <= 'Z')
      c = static_cast<char>(c - 'A' + 'a');
    if (c == '_' || c == ' ')
      c = '-';
    key[j++] = c;
  }
  key[j] = '\0';
  struct Map {
    const char *name;
    const char *glyph;
  };
  static const Map MAP[] = {
      {"sunny", "\ue81a"},
      {"clear", "\ue81a"},
      {"clear-night", "\uf159"},
      {"cloudy", "\uf15c"},
      {"overcast", "\uf15c"},
      {"partlycloudy", "\uf172"},
      {"partly-cloudy", "\uf172"},
      {"partly-cloudy-day", "\uf172"},
      {"partly-cloudy-night", "\uf174"},
      {"rainy", "\uf176"},
      {"rain", "\uf176"},
      {"pouring", "\uf61f"},
      {"snowy", "\ue2cd"},
      {"snow", "\ue2cd"},
      {"snowy-rainy", "\ue2cd"},
      {"hail", "\uf67f"},
      {"lightning", "\uebdb"},
      {"lightning-rainy", "\uebdb"},
      {"thunder", "\uebdb"},
      {"fog", "\ue818"},
      {"foggy", "\ue818"},
      {"mist", "\ue818"},
      {"windy", "\uefd8"},
      {"windy-variant", "\uefd8"},
      {"exceptional", "\uf8b6"},
  };
  for (const auto &row : MAP) {
    if (strcmp(key, row.name) == 0)
      return row.glyph;
  }
  return "\uf15c";
}

void weather_key(const char *condition, char *key, size_t key_size) {
  if (key == nullptr || key_size == 0)
    return;
  size_t j = 0;
  const char *in = condition == nullptr ? "" : condition;
  if (strncmp(in, "mdi:weather-", 12) == 0)
    in += 12;
  else if (strncmp(in, "weather-", 8) == 0)
    in += 8;
  for (; *in != '\0' && j + 1 < key_size; in++) {
    char c = *in;
    if (c >= 'A' && c <= 'Z')
      c = static_cast<char>(c - 'A' + 'a');
    if (c == '_' || c == ' ')
      c = '-';
    key[j++] = c;
  }
  key[j] = '\0';
  // Match Python normalize_weather_key: empty → cloudy so custom themes resolve
  // before the HA text sensor has synced (otherwise Material fallback wins).
  if (j == 0 && key_size > 6) {
    key[0] = 'c';
    key[1] = 'l';
    key[2] = 'o';
    key[3] = 'u';
    key[4] = 'd';
    key[5] = 'y';
    key[6] = '\0';
  }
}

std::string weather_label(const char *condition) {
  char key[48];
  weather_key(condition, key, sizeof(key));
  struct Map {
    const char *name;
    const char *label;
  };
  static const Map MAP[] = {
      {"sunny", "Sunny"},
      {"clear", "Clear"},
      {"clear-night", "Clear night"},
      {"cloudy", "Cloudy"},
      {"overcast", "Overcast"},
      {"partlycloudy", "Mixed"},
      {"partly-cloudy", "Mixed"},
      {"partly-cloudy-day", "Mixed"},
      {"partly-cloudy-night", "Mixed"},
      {"rainy", "Rainy"},
      {"rain", "Rainy"},
      {"pouring", "Pouring"},
      {"snowy", "Snowy"},
      {"snow", "Snowy"},
      {"snowy-rainy", "Snowy rainy"},
      {"hail", "Hail"},
      {"lightning", "Lightning"},
      {"lightning-rainy", "Lightning rainy"},
      {"thunder", "Thunder"},
      {"fog", "Fog"},
      {"foggy", "Foggy"},
      {"mist", "Mist"},
      {"windy", "Windy"},
      {"windy-variant", "Windy"},
      {"exceptional", "Exceptional"},
  };
  for (const auto &row : MAP) {
    if (strcmp(key, row.name) == 0)
      return row.label;
  }
  std::string out;
  bool cap = true;
  for (const char *p = key; *p != '\0'; p++) {
    char c = *p;
    if (c == '-') {
      out.push_back(' ');
      cap = true;
      continue;
    }
    if (cap && c >= 'a' && c <= 'z')
      c = static_cast<char>(c - 'a' + 'A');
    out.push_back(c);
    cap = false;
  }
  return out.empty() ? "Cloudy" : out;
}

const char *compass_point(float bearing) {
  static const char *POINTS[] = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"};
  if (!std::isfinite(bearing))
    return "";
  float wrapped = std::fmod(bearing, 360.0f);
  if (wrapped < 0)
    wrapped += 360.0f;
  int idx = static_cast<int>(std::floor((wrapped + 22.5f) / 45.0f)) % 8;
  return POINTS[idx];
}

uint32_t DateWidget::tick_period_ms() const {
  const uint32_t anim = Widget::tick_period_ms();
  const uint32_t period = this->time_valid_ ? 60000 : 1000;
  if (anim == 0)
    return period;
  return std::min(anim, period);
}

bool DateWidget::refresh_date_(uint32_t now_ms) {
  (void) now_ms;
  int day = 0, month = 0, year = 0;
  bool valid = false;
#ifdef USE_TIME
  if (this->time_ != nullptr) {
    auto now = this->time_->now();
    valid = now.is_valid();
    day = now.day_of_month;
    month = now.month;
    year = now.year;
  }
#endif
  if (valid == this->time_valid_ && day == this->day_ && month == this->month_ && year == this->year_)
    return false;
  this->time_valid_ = valid;
  this->day_ = day;
  this->month_ = month;
  this->year_ = year;
  if (!valid) {
    snprintf(this->label_, sizeof(this->label_), "--");
    snprintf(this->weekday_, sizeof(this->weekday_), "--");
    snprintf(this->month_label_, sizeof(this->month_label_), "--");
    return true;
  }
#ifdef USE_TIME
  auto now = this->time_->now();
  const char *fmt = this->format_.empty() ? "%a %d %b" : this->format_.c_str();
  if (this->show_year_ && this->format_.empty())
    fmt = "%a %d %b %Y";
  now.strftime(this->label_, sizeof(this->label_), fmt);
  now.strftime(this->weekday_, sizeof(this->weekday_), "%a");
  now.strftime(this->month_label_, sizeof(this->month_label_), this->show_year_ ? "%b %y" : "%b");
  if (this->style_ == DateStyle::TWO_LINE) {
    now.strftime(this->label_, sizeof(this->label_), this->show_year_ ? "%d %b %Y" : "%d %b");
  }
  if (this->uppercase_) {
    ascii_upper(this->label_);
    ascii_upper(this->weekday_);
    ascii_upper(this->month_label_);
  }
#endif
  return true;
}

bool DateWidget::prepare(uint32_t now_ms) {
  bool changed = Widget::prepare(now_ms);
  changed = this->refresh_date_(now_ms) || changed;
  return changed;
}

int DateWidget::intrinsic_width() const {
  if (this->width_ > 0)
    return this->width_;
  if (this->style_ == DateStyle::CALENDAR)
    return 24;
  int w = 56;
#ifdef USE_FONT
  if (this->font_ != nullptr)
    w = this->style_ == DateStyle::TWO_LINE ? 48 : 56;
#endif
  if (this->style_ != DateStyle::CALENDAR)
    w += this->icons_.extra_width();
  return w;
}

int DateWidget::intrinsic_height() const {
  if (this->height_ > 0)
    return this->height_;
  if (this->style_ == DateStyle::CALENDAR)
    return 22;
#ifdef USE_FONT
  if (this->font_ != nullptr) {
    int h = this->font_->get_height();
    return this->style_ == DateStyle::TWO_LINE ? std::max(h * 2 + 1, this->icons_.slot_height())
                                              : std::max(h, this->icons_.slot_height());
  }
#endif
  return 12;
}

void DateWidget::draw(DrawContext &ctx, uint32_t now_ms) {
  this->ensure_anim_started_(now_ms);
  const uint8_t a = this->anim_opacity_(now_ms, this->opacity_);
  int dx = 0, dy = 0;
  this->anim_offset_(now_ms, &dx, &dy);
  const int x = this->box_x_ + dx;
  const int y = this->box_y_ + dy;
  const bool calendar = this->style_ == DateStyle::CALENDAR;
#ifdef USE_FONT
  const int h = this->font_ != nullptr ? this->font_->get_height() : 12;
  const int content_h = this->style_ == DateStyle::TWO_LINE ? h * 2 + 1 : h;
  int text_w = 0;
  int ink_h = content_h;
  if (!calendar && this->font_ != nullptr) {
    int w = 0, w2 = 0, x_off = 0, baseline = 0, mh = 0;
    if (this->style_ == DateStyle::TWO_LINE) {
      this->font_->measure(this->weekday_, &w, &x_off, &baseline, &mh);
      this->font_->measure(this->label_, &w2, &x_off, &baseline, &mh);
      text_w = std::max(w, w2);
    } else {
      this->font_->measure(this->label_, &text_w, &x_off, &baseline, &mh);
      if (baseline > 0)
        ink_h = std::min(content_h, baseline);
    }
  }
#endif
  paint_with_outline(this->outline_, true, [&](int ox, int oy, const Color *ink) {
    const Color fill = ink != nullptr ? *ink : this->color_;
    const Color icon_c = ink != nullptr ? *ink : this->icons_.color_;
    int px = x + ox;
    int py = y + oy;
    if (calendar) {
      const ThemeMetrics &m = theme_metrics(ClockTheme::BLOCK);
#ifdef USE_FONT
      ctx.draw_text(px, py, this->font_, fill, a, this->month_label_);
      py += this->font_ != nullptr ? this->font_->get_height() : 6;
#endif
      const int tens = this->time_valid_ ? this->day_ / 10 : -1;
      const int ones = this->time_valid_ ? this->day_ % 10 : -1;
      draw_grid_digit(ctx, px, py, tens, m, fill, a);
      draw_grid_digit(ctx, px + m.digit_w + m.gap, py, ones, m, fill, a);
      return;
    }
#ifdef USE_FONT
    const int box_h = std::max({content_h, this->icons_.slot_height(), this->height_ > 0 ? this->height_ : 0});
    const int ty = py + align_in_box(this->text_align_, 0, box_h, ink_h);
    px = this->icons_.draw_start(ctx, px, py, box_h, a, icon_c);
    if (this->style_ == DateStyle::TWO_LINE) {
      ctx.draw_text(px, ty, this->font_, fill, a, this->weekday_);
      ctx.draw_text(px, ty + h + 1, this->font_, fill, a, this->label_);
    } else {
      ctx.draw_text(px, ty, this->font_, fill, a, this->label_);
    }
    this->icons_.draw_end(ctx, px + text_w, py, box_h, a, icon_c);
#else
    (void) px;
    (void) py;
    (void) fill;
    (void) icon_c;
#endif
  });
}

void WeatherWidget::apply_condition_(const char *condition) {
  char key[sizeof(this->condition_key_)];
  weather_key(condition, key, sizeof(key));
  const char *glyph = weather_glyph(condition);
  std::string label = weather_label(condition);
  // Look up by key at use-time — CustomIconEntry pointers dangle after vector growth.
  if (strcmp(this->condition_key_, key) == 0 && this->glyph_ == glyph && this->label_ == label)
    return;
  memcpy(this->condition_key_, key, sizeof(this->condition_key_));
  this->glyph_ = glyph;
  this->label_ = label;
  this->mark_dirty();
}

void WeatherWidget::rebind_custom_icon_ptrs_() {
  for (auto &entry : this->custom_icons_) {
    entry.icon.pixels = entry.pixel_storage.empty() ? nullptr : entry.pixel_storage.data();
    entry.icon.palette = entry.palette_storage.empty() ? nullptr : entry.palette_storage.data();
    entry.icon.palette_count = entry.palette_storage.size();
  }
}

void WeatherWidget::add_custom_icon(const std::string &key, const uint8_t *pixels, int width, int height, Color color,
                                    const Color *palette, size_t palette_count) {
  CustomIconEntry entry;
  entry.key = key;
  entry.icon.width = width;
  entry.icon.height = height;
  entry.icon.color = color;
  const size_t nbytes =
      (static_cast<size_t>(std::max(0, width)) * static_cast<size_t>(std::max(0, height)) + 1) / 2;
  if (pixels != nullptr && nbytes > 0)
    entry.pixel_storage.assign(pixels, pixels + nbytes);
  if (palette != nullptr && palette_count > 0) {
    entry.palette_storage.reserve(palette_count);
    for (size_t i = 0; i < palette_count; i++) {
      const Color &c = palette[i];
      entry.palette_storage.emplace_back(c.r, c.g, c.b, c.w);
    }
  }
  this->custom_icons_.push_back(std::move(entry));
  this->rebind_custom_icon_ptrs_();
}

static const char *weather_icon_alias(const char *key) {
  struct Map {
    const char *from;
    const char *to;
  };
  static const Map ALIASES[] = {
      {"clear", "sunny"},
      {"overcast", "cloudy"},
      {"partly-cloudy", "partlycloudy"},
      {"partly-cloudy-day", "partlycloudy"},
      {"partly-cloudy-night", "partlycloudy"},
      {"rain", "rainy"},
      {"snow", "snowy"},
      {"snowy-rainy", "snowy"},
      {"lightning-rainy", "lightning"},
      {"thunder", "lightning"},
      {"foggy", "fog"},
      {"mist", "fog"},
      {"windy-variant", "windy"},
  };
  for (const auto &row : ALIASES) {
    if (strcmp(key, row.from) == 0)
      return row.to;
  }
  return nullptr;
}

const WeatherCustomIcon *WeatherWidget::find_custom_icon_(const char *key) const {
  if (this->custom_icons_.empty())
    return nullptr;
  const char *lookup = (key == nullptr || key[0] == '\0') ? "cloudy" : key;
  for (const auto &entry : this->custom_icons_) {
    if (entry.key == lookup)
      return &entry.icon;
  }
  const char *alias = weather_icon_alias(lookup);
  if (alias != nullptr) {
    for (const auto &entry : this->custom_icons_) {
      if (entry.key == alias)
        return &entry.icon;
    }
  }
  for (const auto &entry : this->custom_icons_) {
    if (entry.key == "default")
      return &entry.icon;
  }
  return nullptr;
}

static uint8_t weather_icon_index_at(const WeatherCustomIcon *icon, int x, int y) {
  if (icon == nullptr || icon->pixels == nullptr || x < 0 || y < 0 || x >= icon->width || y >= icon->height)
    return 0;
  const size_t i = static_cast<size_t>(y) * static_cast<size_t>(icon->width) + static_cast<size_t>(x);
  const uint8_t b = icon->pixels[i >> 1];
  return (i & 1) ? static_cast<uint8_t>(b & 0x0F) : static_cast<uint8_t>(b >> 4);
}

static Color weather_icon_color_for(const WeatherCustomIcon *icon, uint8_t idx) {
  if (idx <= 1)
    return icon->color;
  const int extra = static_cast<int>(idx) - 2;
  if (extra >= 0 && extra < static_cast<int>(icon->palette_count))
    return icon->palette[static_cast<size_t>(extra)];
  return icon->color;
}

void WeatherWidget::draw_custom_icon_(DrawContext &ctx, int x0, int y0, const WeatherCustomIcon *icon, uint8_t a,
                                    const Color *ink) const {
  if (icon == nullptr || icon->pixels == nullptr)
    return;
  for (int y = 0; y < icon->height; y++) {
    for (int x = 0; x < icon->width; x++) {
      const uint8_t idx = weather_icon_index_at(icon, x, y);
      if (idx == 0)
        continue;
      const Color col = ink != nullptr ? *ink : weather_icon_color_for(icon, idx);
      ctx.blend_pixel(x0 + x, y0 + y, col, a);
    }
  }
}

void WeatherWidget::bind(PixelLayout *host) {
  Widget::bind(host);
#ifdef USE_TEXT_SENSOR
  if (this->condition_sensor_ != nullptr) {
    this->condition_sensor_->add_on_state_callback(
        [this](const std::string &value) { this->apply_condition_(value.c_str()); });
    if (this->condition_sensor_->has_state())
      this->apply_condition_(this->condition_sensor_->state.c_str());
  }
#endif
#ifdef USE_SENSOR
  if (this->temperature_sensor_ != nullptr) {
    this->temperature_sensor_->add_on_state_callback([this](float) { this->mark_dirty(); });
  }
  if (this->humidity_sensor_ != nullptr) {
    this->humidity_sensor_->add_on_state_callback([this](float) { this->mark_dirty(); });
  }
  if (this->wind_speed_sensor_ != nullptr) {
    this->wind_speed_sensor_->add_on_state_callback([this](float) { this->mark_dirty(); });
  }
  if (this->wind_bearing_sensor_ != nullptr) {
    this->wind_bearing_sensor_->add_on_state_callback([this](float) { this->mark_dirty(); });
  }
#endif
  if (this->glyph_.empty())
    this->apply_condition_("");
}

int WeatherWidget::icon_size_() const {
  const WeatherCustomIcon *icon = this->find_custom_icon_(this->condition_key_);
  if (icon != nullptr)
    return std::max(icon->width, icon->height);
#ifdef USE_FONT
  if (this->icon_font_ != nullptr)
    return this->icon_font_->get_height();
#endif
  return 18;
}

int WeatherWidget::line_height_() const {
#ifdef USE_FONT
  if (this->font_ != nullptr)
    return this->font_->get_height();
#endif
  return 12;
}

int WeatherWidget::text_ink_height_() const {
#ifdef USE_FONT
  if (this->font_ == nullptr)
    return this->text_block_height_();
  char temp[24];
  char humidity[16];
  char wind[40];
  this->format_temp_(temp, sizeof(temp));
  this->format_humidity_(humidity, sizeof(humidity));
  this->format_wind_(wind, sizeof(wind));
  const char *lines[4];
  int line_count = 0;
  if (this->show_condition_ && !this->label_.empty())
    lines[line_count++] = this->label_.c_str();
  if (this->show_temp_ && temp[0] != '\0')
    lines[line_count++] = temp;
  if (this->show_humidity_ && humidity[0] != '\0')
    lines[line_count++] = humidity;
  if (this->show_wind_ && wind[0] != '\0')
    lines[line_count++] = wind;
  if (line_count == 0)
    return 0;
  const int cell = this->line_height_();
  int ink_top = cell;
  int ink_bottom = 0;
  for (int i = 0; i < line_count; i++) {
    int w = 0, x_off = 0, baseline = 0, h = 0;
    this->font_->measure(lines[i], &w, &x_off, &baseline, &h);
    const int y = i * (cell + 1);
    const int line_ink = baseline > 0 ? std::min(baseline, h > 0 ? h : cell) : (h > 0 ? h : cell);
    ink_top = std::min(ink_top, y);
    ink_bottom = std::max(ink_bottom, y + line_ink);
  }
  return std::max(1, ink_bottom - ink_top);
#else
  return this->text_block_height_();
#endif
}

int WeatherWidget::measure_line_(const char *text) const {
  if (text == nullptr || text[0] == '\0')
    return 0;
#ifdef USE_FONT
  if (this->font_ != nullptr) {
    int w = 0, x_off = 0, baseline = 0, h = 0;
    this->font_->measure(text, &w, &x_off, &baseline, &h);
    return w;
  }
#endif
  return static_cast<int>(strlen(text) * 6);
}

void WeatherWidget::format_temp_(char *out, size_t out_size) const {
  out[0] = '\0';
#ifdef USE_SENSOR
  if (this->temperature_sensor_ == nullptr || !this->temperature_sensor_->has_state() ||
      !std::isfinite(this->temperature_sensor_->state))
    return;
  const float temp = this->temperature_sensor_->state;
  std::string unit = this->temperature_sensor_->get_unit_of_measurement();
  if (unit.empty())
    unit = "°C";
  const bool tight = !unit.empty() && !std::isalnum(static_cast<unsigned char>(unit[0]));
  if (tight) {
    if (std::fabs(temp - std::round(temp)) < 0.05f)
      snprintf(out, out_size, "%d%s", static_cast<int>(std::lround(temp)), unit.c_str());
    else
      snprintf(out, out_size, "%.1f%s", temp, unit.c_str());
  } else if (std::fabs(temp - std::round(temp)) < 0.05f) {
    snprintf(out, out_size, "%d %s", static_cast<int>(std::lround(temp)), unit.c_str());
  } else {
    snprintf(out, out_size, "%.1f %s", temp, unit.c_str());
  }
#else
  (void) out_size;
#endif
}

void WeatherWidget::format_humidity_(char *out, size_t out_size) const {
  out[0] = '\0';
#ifdef USE_SENSOR
  if (this->humidity_sensor_ == nullptr || !this->humidity_sensor_->has_state() ||
      !std::isfinite(this->humidity_sensor_->state))
    return;
  const float humidity = this->humidity_sensor_->state;
  snprintf(out, out_size, "%d%%", static_cast<int>(std::lround(humidity)));
#else
  (void) out_size;
#endif
}

void WeatherWidget::format_wind_(char *out, size_t out_size) const {
  out[0] = '\0';
#ifdef USE_SENSOR
  bool have_speed = this->wind_speed_sensor_ != nullptr && this->wind_speed_sensor_->has_state() &&
                    std::isfinite(this->wind_speed_sensor_->state);
  const char *dir = "";
  if (this->wind_bearing_sensor_ != nullptr && this->wind_bearing_sensor_->has_state())
    dir = compass_point(this->wind_bearing_sensor_->state);
  if (!have_speed && dir[0] == '\0')
    return;
  if (have_speed) {
    const float speed = this->wind_speed_sensor_->state;
    std::string unit = this->wind_speed_sensor_->get_unit_of_measurement();
    if (unit.empty())
      unit = "km/h";
    if (dir[0] != '\0') {
      if (std::fabs(speed - std::round(speed)) < 0.05f)
        snprintf(out, out_size, "%d %s %s", static_cast<int>(std::lround(speed)), unit.c_str(), dir);
      else
        snprintf(out, out_size, "%.1f %s %s", speed, unit.c_str(), dir);
    } else if (std::fabs(speed - std::round(speed)) < 0.05f) {
      snprintf(out, out_size, "%d %s", static_cast<int>(std::lround(speed)), unit.c_str());
    } else {
      snprintf(out, out_size, "%.1f %s", speed, unit.c_str());
    }
  } else {
    snprintf(out, out_size, "%s", dir);
  }
#else
  (void) out_size;
#endif
}

int WeatherWidget::text_block_width_() const {
  char temp[24];
  char humidity[16];
  char wind[40];
  this->format_temp_(temp, sizeof(temp));
  this->format_humidity_(humidity, sizeof(humidity));
  this->format_wind_(wind, sizeof(wind));
  int text_w = 0;
  if (this->show_condition_ && !this->label_.empty())
    text_w = std::max(text_w, this->measure_line_(this->label_.c_str()));
  if (this->show_temp_ && temp[0] != '\0')
    text_w = std::max(text_w, this->measure_line_(temp));
  if (this->show_humidity_ && humidity[0] != '\0')
    text_w = std::max(text_w, this->measure_line_(humidity));
  if (this->show_wind_ && wind[0] != '\0')
    text_w = std::max(text_w, this->measure_line_(wind));
  return text_w;
}

int WeatherWidget::text_block_height_() const {
  char temp[24];
  char humidity[16];
  char wind[40];
  this->format_temp_(temp, sizeof(temp));
  this->format_humidity_(humidity, sizeof(humidity));
  this->format_wind_(wind, sizeof(wind));
  int lines = 0;
  if (this->show_condition_ && !this->label_.empty())
    lines++;
  if (this->show_temp_ && temp[0] != '\0')
    lines++;
  if (this->show_humidity_ && humidity[0] != '\0')
    lines++;
  if (this->show_wind_ && wind[0] != '\0')
    lines++;
  if (lines == 0)
    return 0;
  return lines * this->line_height_() + (lines - 1);
}

int WeatherWidget::intrinsic_width() const {
  if (this->width_ > 0)
    return this->width_;
  const int icon_w = this->show_icon_ ? this->icon_size_() : 0;
  const int text_w = this->text_block_width_();
  const bool side =
      this->text_position_ == WeatherTextPosition::END || this->text_position_ == WeatherTextPosition::START;
  if (side) {
    if (icon_w && text_w)
      return icon_w + this->gap_ + text_w;
    if (icon_w)
      return icon_w;
    if (text_w)
      return text_w;
  } else {
    const int w = std::max(icon_w, text_w);
    if (w > 0)
      return w;
  }
  return 18;
}

int WeatherWidget::intrinsic_height() const {
  if (this->height_ > 0)
    return this->height_;
  const int icon_h = this->show_icon_ ? this->icon_size_() : 0;
  const int text_h = this->text_block_height_();
  const bool side =
      this->text_position_ == WeatherTextPosition::END || this->text_position_ == WeatherTextPosition::START;
  if (side) {
    const int h = std::max(icon_h, text_h);
    return h > 0 ? h : 18;
  }
  if (icon_h && text_h)
    return icon_h + this->gap_ + text_h;
  if (icon_h)
    return icon_h;
  if (text_h)
    return text_h;
  return 18;
}

struct IconTextLayout {
  int icon_x;
  int icon_y;
  int text_x;
  int text_y;
};

static IconTextLayout layout_icon_text(WeatherTextPosition position, IconAlign icon_align, IconAlign text_align,
                                       int gap, int box_w, int box_h, int icon_w, int icon_h, int text_w, int text_h,
                                       int origin_x, int origin_y) {
  IconTextLayout out{};
  const bool side = position == WeatherTextPosition::END || position == WeatherTextPosition::START;
  if (side) {
    const int track = std::max({icon_h, text_h, box_h});
    out.icon_y = origin_y + align_in_box(icon_align, 0, track, icon_h);
    out.text_y = origin_y + align_in_box(text_align, 0, track, text_h);
    if (position == WeatherTextPosition::END) {
      out.icon_x = origin_x;
      out.text_x = origin_x + (icon_w ? icon_w + gap : 0);
    } else {
      out.text_x = origin_x;
      out.icon_x = origin_x + (text_w ? text_w + gap : 0);
    }
  } else {
    const int track = std::max({icon_w, text_w, box_w});
    out.icon_x = origin_x + align_in_box(icon_align, 0, track, icon_w);
    out.text_x = origin_x + align_in_box(text_align, 0, track, text_w);
    if (position == WeatherTextPosition::BELOW) {
      out.icon_y = origin_y;
      out.text_y = origin_y + (icon_h ? icon_h + gap : 0);
    } else {
      out.text_y = origin_y;
      out.icon_y = origin_y + (text_h ? text_h + gap : 0);
    }
  }
  return out;
}

void WeatherWidget::draw(DrawContext &ctx, uint32_t now_ms) {
  this->ensure_anim_started_(now_ms);
  const uint8_t a = this->anim_opacity_(now_ms, this->opacity_);
  int dx = 0, dy = 0;
  this->anim_offset_(now_ms, &dx, &dy);
  const int x = this->box_x_ + dx;
  const int y = this->box_y_ + dy;
#ifdef USE_FONT
  char temp[24];
  char humidity[16];
  char wind[40];
  this->format_temp_(temp, sizeof(temp));
  this->format_humidity_(humidity, sizeof(humidity));
  this->format_wind_(wind, sizeof(wind));
  const char *lines[4];
  int line_count = 0;
  if (this->show_condition_ && !this->label_.empty())
    lines[line_count++] = this->label_.c_str();
  if (this->show_temp_ && temp[0] != '\0')
    lines[line_count++] = temp;
  if (this->show_humidity_ && humidity[0] != '\0')
    lines[line_count++] = humidity;
  if (this->show_wind_ && wind[0] != '\0')
    lines[line_count++] = wind;
  const int icon_w = this->show_icon_ ? this->icon_size_() : 0;
  const int icon_h = icon_w;
  const int text_w = this->text_block_width_();
  const int text_h = this->text_ink_height_();
  const int lh = this->line_height_();
  const int box_w = this->width_ > 0 ? this->width_ : this->intrinsic_width();
  const int box_h = this->height_ > 0 ? this->height_ : this->intrinsic_height();
  paint_with_outline(this->outline_, true, [&](int ox, int oy, const Color *ink) {
    const Color fill = ink != nullptr ? *ink : this->color_;
    const IconTextLayout layout =
        layout_icon_text(this->text_position_, this->icon_align_, this->text_align_, this->gap_, box_w, box_h, icon_w,
                         icon_h, text_w, text_h, x + ox, y + oy);
    const int icon_x = layout.icon_x;
    const int icon_y = layout.icon_y;
    const int text_x = layout.text_x;
    const int text_y = layout.text_y;
    if (this->show_icon_) {
      const WeatherCustomIcon *icon = this->find_custom_icon_(this->condition_key_);
      if (icon != nullptr) {
        this->draw_custom_icon_(ctx, icon_x, icon_y, icon, a, ink);
      } else {
        font::Font *face = this->icon_font_ != nullptr ? this->icon_font_ : this->font_;
        ctx.draw_text(icon_x, icon_y, face, fill, a, this->glyph_.c_str());
      }
    }
    int ty = text_y;
    for (int i = 0; i < line_count; i++) {
      ctx.draw_text(text_x, ty, this->font_, fill, a, lines[i]);
      ty += lh + (i + 1 < line_count ? 1 : 0);
    }
  });
#else
  (void) ctx;
  (void) a;
  (void) x;
  (void) y;
#endif
}

int SpriteWidget::intrinsic_width() const {
  if (this->width_ > 0)
    return this->width_;
  if (this->frame_width_ > 0)
    return this->frame_width_;
#ifdef USE_ANIMATION
  if (this->animation_ != nullptr)
    return this->animation_->get_width();
#endif
#ifdef USE_IMAGE
  if (this->image_ != nullptr)
    return this->image_->get_width();
#endif
  return 16;
}

int SpriteWidget::intrinsic_height() const {
  if (this->height_ > 0)
    return this->height_;
  if (this->frame_height_ > 0)
    return this->frame_height_;
#ifdef USE_ANIMATION
  if (this->animation_ != nullptr)
    return this->animation_->get_height();
#endif
#ifdef USE_IMAGE
  if (this->image_ != nullptr)
    return this->image_->get_height();
#endif
  return 16;
}

uint32_t SpriteWidget::tick_period_ms() const {
  const uint32_t anim = Widget::tick_period_ms();
  uint32_t period = 0;
  if (this->fps_ > 0) {
#ifdef USE_ANIMATION
    if (this->animation_ != nullptr)
      period = static_cast<uint32_t>(1000.0f / this->fps_);
#endif
    if (this->frames_ > 1 || this->frame_width_ > 0)
      period = static_cast<uint32_t>(1000.0f / this->fps_);
  }
  if (period == 0)
    return anim;
  if (anim == 0)
    return period;
  return std::min(anim, period);
}

bool SpriteWidget::prepare(uint32_t now_ms) {
  bool changed = Widget::prepare(now_ms);
  const uint32_t interval = this->fps_ <= 0 ? 0 : static_cast<uint32_t>(1000.0f / this->fps_);
  if (interval == 0)
    return changed;
  if (this->last_frame_ms_ == 0)
    this->last_frame_ms_ = now_ms;
  if (now_ms - this->last_frame_ms_ < interval)
    return changed;
  this->last_frame_ms_ = now_ms;
#ifdef USE_ANIMATION
  if (this->animation_ != nullptr) {
    this->animation_->next_frame();
    return true;
  }
#endif
  const int n = this->frames_ > 0 ? this->frames_ : 1;
  if (n <= 1)
    return changed;
  int next = this->current_frame_ + 1;
  if (next >= n)
    next = this->loop_ ? 0 : n - 1;
  if (next == this->current_frame_)
    return changed;
  this->current_frame_ = next;
  return true;
}

void SpriteWidget::draw(DrawContext &ctx, uint32_t now_ms) {
  this->ensure_anim_started_(now_ms);
  const uint8_t a = this->anim_opacity_(now_ms, this->opacity_);
  int dx = 0, dy = 0;
  this->anim_offset_(now_ms, &dx, &dy);
  const int x = this->box_x_ + dx;
  const int y = this->box_y_ + dy;
#ifdef USE_ANIMATION
  if (this->animation_ != nullptr) {
#ifdef USE_IMAGE
    ctx.draw_image_region(this->animation_, x, y, 0, 0, this->animation_->get_width(), this->animation_->get_height(),
                          a);
#endif
    return;
  }
#endif
#ifdef USE_IMAGE
  if (this->image_ != nullptr) {
    const int fw = this->frame_width_ > 0 ? this->frame_width_ : this->image_->get_width();
    const int fh = this->frame_height_ > 0 ? this->frame_height_ : this->image_->get_height();
    const int cols = std::max(1, this->image_->get_width() / fw);
    const int frame = this->current_frame_;
    const int src_x = (frame % cols) * fw;
    const int src_y = (frame / cols) * fh;
    ctx.draw_image_region(this->image_, x, y, src_x, src_y, fw, fh, a);
  }
#else
  (void) ctx;
  (void) a;
  (void) x;
  (void) y;
#endif
}

int CustomWidget::intrinsic_width() const {
  if (this->width_ > 0)
    return this->width_;
  return this->pix_w_ > 0 ? this->pix_w_ : 8;
}

int CustomWidget::intrinsic_height() const {
  if (this->height_ > 0)
    return this->height_;
  return this->pix_h_ > 0 ? this->pix_h_ : 8;
}

uint8_t CustomWidget::index_at_(int x, int y) const {
  if (this->pixels_ == nullptr || x < 0 || y < 0 || x >= this->pix_w_ || y >= this->pix_h_)
    return 0;
  const size_t i = static_cast<size_t>(y) * static_cast<size_t>(this->pix_w_) + static_cast<size_t>(x);
  const uint8_t b = this->pixels_[i >> 1];
  return (i & 1) ? static_cast<uint8_t>(b & 0x0F) : static_cast<uint8_t>(b >> 4);
}

Color CustomWidget::color_for_index_(uint8_t idx) const {
  if (idx <= 1)
    return this->color_;
  const int extra = static_cast<int>(idx) - 2;
  if (extra >= 0 && extra < static_cast<int>(this->palette_.size()))
    return this->palette_[static_cast<size_t>(extra)];
  return this->color_;
}

void CustomWidget::draw(DrawContext &ctx, uint32_t now_ms) {
  this->ensure_anim_started_(now_ms);
  const uint8_t a = this->anim_opacity_(now_ms, this->opacity_);
  int dx = 0, dy = 0;
  this->anim_offset_(now_ms, &dx, &dy);
  const int x0 = this->box_x_ + dx;
  const int y0 = this->box_y_ + dy;
  const int max_w = this->box_w_ > 0 ? this->box_w_ : this->intrinsic_width();
  const int max_h = this->box_h_ > 0 ? this->box_h_ : this->intrinsic_height();
  const int cols = std::min(max_w, this->pix_w_);
  const int rows = std::min(max_h, this->pix_h_);
  for (int y = 0; y < rows; y++) {
    for (int x = 0; x < cols; x++) {
      const uint8_t idx = this->index_at_(x, y);
      if (idx == 0)
        continue;
      ctx.blend_pixel(x0 + x, y0 + y, this->color_for_index_(idx), a);
    }
  }
}

}  // namespace pixel_layout
}  // namespace esphome
