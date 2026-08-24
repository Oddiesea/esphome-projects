#pragma once

#include <cmath>
#include <cstddef>
#include <functional>
#include <string>
#include <vector>

#include "esphome/core/color.h"
#include "esphome/core/component.h"
#include "esphome/components/display/display.h"

#ifdef USE_PIXEL_LAYOUT_SD_STORAGE
#include "esphome/components/pixel_layout/sd_storage.h"
#endif

#ifdef USE_FONT
#include "esphome/components/font/font.h"
#endif
#ifdef USE_IMAGE
#include "esphome/components/image/image.h"
#endif
#ifdef USE_ANIMATION
#include "esphome/components/animation/animation.h"
#endif
#ifdef USE_TIME
#include "esphome/components/time/real_time_clock.h"
#endif
#ifdef USE_SENSOR
#include "esphome/components/sensor/sensor.h"
#endif
#ifdef USE_TEXT_SENSOR
#include "esphome/components/text_sensor/text_sensor.h"
#endif

namespace esphome {
namespace pixel_layout {

enum class AlignMain : uint8_t { START, CENTER, END, SPACE_BETWEEN };
enum class AlignCross : uint8_t { START, CENTER, END };
enum class ClockFace : uint8_t { DIGITAL, ANALOG };
enum class ClockTheme : uint8_t {
  SEVEN_SEGMENT,
  ROUNDED,
  BLOCK,
  TINY,
  TYPEFACE,
  SPLIT_FLAP,
  PERSPECTIVE,
  RING,
  MINIMAL,
  TICKS,
  SQUARE
};
enum class ClockSize : uint8_t { SM, MD, LG };
enum class Outline : uint8_t { NONE, BLACK, WHITE };
enum class IconAlign : uint8_t { TOP, MIDDLE, BOTTOM };
enum class DateStyle : uint8_t { TEXT, TWO_LINE, CALENDAR };
enum class TextStyle : uint8_t { TEXT, TWO_LINE };
using SensorStyle = TextStyle;
enum class WeatherTextPosition : uint8_t { END, START, BELOW, ABOVE };
enum class BoxShape : uint8_t { RECT, ROUNDED, OVAL, PILL, TRIANGLE, DIAMOND, PLUS, FRAME, RING, LINE };
enum class BoxPoint : uint8_t { UP, DOWN, LEFT, RIGHT };
enum class AnimType : uint8_t { NONE, FADE, SLIDE, PULSE, BLINK };
enum class AnimMode : uint8_t { IN, OUT, IN_OUT };
enum class ScreenTransition : uint8_t {
  CUT,
  FADE,
  SLIDE_LEFT,
  SLIDE_RIGHT,
  SLIDE_UP,
  SLIDE_DOWN,
  WIPE_LEFT,
  WIPE_RIGHT,
  WIPE_UP,
  WIPE_DOWN,
  IRIS,
  DISSOLVE,
  BLINDS
};
const char *weather_glyph(const char *condition);
std::string weather_label(const char *condition);
void weather_key(const char *condition, char *key, size_t key_size);
const char *compass_point(float bearing);

struct WeatherCustomIcon {
  const uint8_t *pixels{nullptr};
  int width{0};
  int height{0};
  Color color{255, 255, 255};
  const Color *palette{nullptr};
  size_t palette_count{0};
};

struct VisibleClause {
#ifdef USE_SENSOR
  sensor::Sensor *sensor{nullptr};
  uint8_t cmp{0};
  float value{0};
  uint8_t cmp2{0};
  float value2{0};
#endif
#ifdef USE_TEXT_SENSOR
  text_sensor::TextSensor *text{nullptr};
  std::string state;
#endif
  bool invert{false};
  bool matches() const;
};

struct ThemeMetrics {
  uint8_t digit_w;
  uint8_t digit_h;
  uint8_t thickness;
  uint8_t gap;
  uint8_t colon_w;
  uint8_t colon_gap;
  bool rounded;
  uint8_t cell;
  uint8_t cell_gap;
  bool grid;
  // Perspective 7-seg: non-zero thickness_top enables tapered digits (wide top, narrow bottom).
  uint8_t thickness_top;
  uint8_t thickness_mid;
  uint8_t thickness_bot;
  uint8_t taper;
};

const ThemeMetrics &theme_metrics(ClockTheme theme);
ThemeMetrics clock_metrics(ClockTheme theme, ClockSize size);
void measure_digital_clock(ClockTheme theme, ClockSize size, bool show_seconds, int *width, int *height,
                           bool show_colon = true);

class DrawContext;

struct SideIcons {
  void set_start(const std::string &icon) { this->start_ = icon; }
  void set_end(const std::string &icon) { this->end_ = icon; }
  void set_align(IconAlign align) { this->align_ = align; }
  void set_gap(int gap) { this->gap_ = gap < 0 ? 0 : gap > 8 ? 8 : gap; }
  void set_color(Color color) { this->color_ = color; }
#ifdef USE_FONT
  void set_font(font::Font *font) { this->font_ = font; }
#endif
  int extra_width() const;
#ifdef USE_FONT
  int slot_height() const {
    if (this->font_ == nullptr)
      return 0;
    if (this->start_.empty() && this->end_.empty())
      return 0;
    return this->font_->get_height();
  }
#else
  int slot_height() const { return 0; }
#endif
  int draw_start(DrawContext &ctx, int x, int y, int content_h, uint8_t alpha) const {
    return this->draw_start(ctx, x, y, content_h, alpha, this->color_);
  }
  int draw_start(DrawContext &ctx, int x, int y, int content_h, uint8_t alpha, Color color) const;
  void draw_end(DrawContext &ctx, int x, int y, int content_h, uint8_t alpha) const {
    this->draw_end(ctx, x, y, content_h, alpha, this->color_);
  }
  void draw_end(DrawContext &ctx, int x, int y, int content_h, uint8_t alpha, Color color) const;

  std::string start_;
  std::string end_;
  IconAlign align_{IconAlign::MIDDLE};
  int gap_{2};
  Color color_{255, 255, 255};
#ifdef USE_FONT
  font::Font *font_{nullptr};
#endif
};

struct WidgetAnim {
  AnimType type{AnimType::NONE};
  AnimMode mode{AnimMode::IN};
  uint32_t duration_ms{400};
  uint32_t delay_ms{0};
  int repeat{-1};
  int16_t dx{0};
  int16_t dy{0};
  uint8_t from_opacity{0};
  uint8_t to_opacity{255};
  uint32_t start_ms{0};
  bool started{false};
};

class PixelLayout;

class DrawContext {
 public:
  void init(uint8_t *buffer, int width, int height);
  void clear(Color background);
  void blit(display::Display &it) const;
  const uint8_t *data() const { return this->buffer_; }
  void blend_pixel(int x, int y, Color color, uint8_t alpha);
  void fill_rect(int x, int y, int w, int h, Color color, uint8_t alpha);
  void fill_round_rect(int x, int y, int w, int h, int radius, bool antialias, Color color, uint8_t alpha);
  void fill_ellipse(int x, int y, int w, int h, bool antialias, Color color, uint8_t alpha);
  void fill_triangle(int x0, int y0, int x1, int y1, int x2, int y2, bool antialias, Color color, uint8_t alpha);
  void stroke_ellipse(int x, int y, int w, int h, int thickness, bool antialias, Color color, uint8_t alpha);
  void stroke_line(int x0, int y0, int x1, int y1, int thickness, Color color, uint8_t alpha);
  void hline(int x, int y, int w, Color color, uint8_t alpha);
  void vline(int x, int y, int h, Color color, uint8_t alpha);
  void line(int x0, int y0, int x1, int y1, Color color, uint8_t alpha);
  void circle_outline(int cx, int cy, int r, Color color, uint8_t alpha);
#ifdef USE_FONT
  void draw_text(int x, int y, font::Font *font, Color color, uint8_t alpha, const char *text);
  void measure_text(font::Font *font, const char *text, int *width, int *height);
#endif
#ifdef USE_IMAGE
  void draw_image_region(image::Image *img, int dst_x, int dst_y, int src_x, int src_y, int w, int h, uint8_t alpha);
#endif

  int width() const { return this->width_; }
  int height() const { return this->height_; }
  void set_alpha_scale(uint8_t scale) { this->alpha_scale_ = scale; }
  void set_origin(int x, int y) {
    this->origin_x_ = x;
    this->origin_y_ = y;
  }
  void reset_mask();
  void set_clip(int x, int y, int w, int h);
  void set_dissolve(uint16_t limit);
  void set_blinds(uint8_t open, uint8_t period = 8, bool vertical = false);

 protected:
  bool accepts_(int x, int y) const;

  uint8_t *buffer_{nullptr};
  int width_{0};
  int height_{0};
  int origin_x_{0};
  int origin_y_{0};
  uint8_t alpha_scale_{255};
  bool clip_{false};
  int clip_x0_{0};
  int clip_y0_{0};
  int clip_x1_{0};
  int clip_y1_{0};
  bool dissolve_{false};
  uint16_t dissolve_limit_{0};
  uint8_t blinds_period_{0};
  uint8_t blinds_open_{0};
  bool blinds_vertical_{false};
};

class Widget {
 public:
  virtual ~Widget() = default;
  virtual void layout(int origin_x, int origin_y, int avail_w, int avail_h);
  virtual void draw(DrawContext &ctx, uint32_t now_ms) = 0;
  virtual void bind(PixelLayout *host);
  virtual bool prepare(uint32_t now_ms);
  virtual int intrinsic_width() const { return this->width_ > 0 ? this->width_ : 0; }
  virtual int intrinsic_height() const { return this->height_ > 0 ? this->height_ : 0; }
  virtual uint32_t tick_period_ms() const;

  void set_x(int x) { this->x_ = x; }
  void set_y(int y) { this->y_ = y; }
  void set_width(int width) { this->width_ = width; }
  void set_height(int height) { this->height_ = height; }
  void set_opacity(uint8_t opacity) { this->opacity_ = opacity; }
  void set_expanded(bool expanded) { this->expanded_ = expanded; }
  void set_outline(Outline outline) { this->outline_ = outline; }
#ifdef USE_SENSOR
  void add_visible_sensor(sensor::Sensor *sensor, uint8_t cmp, float value, uint8_t cmp2, float value2, bool invert);
#endif
#ifdef USE_TEXT_SENSOR
  void add_visible_text(text_sensor::TextSensor *sensor, const std::string &state, bool invert);
#endif
  void set_visible_match_all(bool match_all) { this->visible_match_all_ = match_all; }
  void set_visible_invert(bool invert) { this->visible_invert_ = invert; }
  bool is_shown() const { return this->shown_; }
  void draw_if_shown(DrawContext &ctx, uint32_t now_ms) {
    if (!this->is_shown())
      return;
    this->draw(ctx, now_ms);
  }
  void set_animation(AnimType type, uint32_t duration_ms, uint32_t delay_ms, int repeat, int16_t dx, int16_t dy,
                     uint8_t from_opacity, uint8_t to_opacity, AnimMode mode = AnimMode::IN) {
    this->anim_.type = type;
    this->anim_.duration_ms = duration_ms;
    this->anim_.delay_ms = delay_ms;
    this->anim_.repeat = repeat;
    this->anim_.dx = dx;
    this->anim_.dy = dy;
    this->anim_.from_opacity = from_opacity;
    this->anim_.to_opacity = to_opacity;
    this->anim_.mode = mode;
  }

  bool expanded() const { return this->expanded_; }
  int offset_x() const { return this->x_; }
  int offset_y() const { return this->y_; }
  int box_x() const { return this->box_x_; }
  int box_y() const { return this->box_y_; }
  int box_w() const { return this->box_w_; }
  int box_h() const { return this->box_h_; }
  void mark_dirty();

 protected:
  void ensure_anim_started_(uint32_t now_ms);
  float anim_amount_(uint32_t now_ms) const;
  uint8_t anim_opacity_(uint32_t now_ms, uint8_t base) const;
  void anim_offset_(uint32_t now_ms, int *dx, int *dy) const;
  bool animation_active_(uint32_t now_ms);
  void recompute_visible_();
  void bind_visible_();

  int x_{0};
  int y_{0};
  int width_{-1};
  int height_{-1};
  int box_x_{0};
  int box_y_{0};
  int box_w_{0};
  int box_h_{0};
  uint8_t opacity_{255};
  bool expanded_{false};
  Outline outline_{Outline::NONE};
  WidgetAnim anim_{};
  PixelLayout *host_{nullptr};
  std::vector<VisibleClause> visible_clauses_{};
  bool visible_match_all_{true};
  bool visible_invert_{false};
  bool shown_{true};
};

class ContainerWidget : public Widget {
 public:
  void add_child(Widget *child) { this->children_.push_back(child); }
  void bind(PixelLayout *host) override;
  bool prepare(uint32_t now_ms) override;
  uint32_t tick_period_ms() const override;

 protected:
  std::vector<Widget *> children_;
};

class StackWidget : public ContainerWidget {
 public:
  void layout(int origin_x, int origin_y, int avail_w, int avail_h) override;
  void draw(DrawContext &ctx, uint32_t now_ms) override;
  int intrinsic_width() const override;
  int intrinsic_height() const override;
};

class RowWidget : public ContainerWidget {
 public:
  void set_gap(int gap) { this->gap_ = gap; }
  void set_main_align(AlignMain align) { this->main_align_ = align; }
  void set_cross_align(AlignCross align) { this->cross_align_ = align; }
  void layout(int origin_x, int origin_y, int avail_w, int avail_h) override;
  void draw(DrawContext &ctx, uint32_t now_ms) override;
  int intrinsic_width() const override;
  int intrinsic_height() const override;

 protected:
  int gap_{0};
  AlignMain main_align_{AlignMain::START};
  AlignCross cross_align_{AlignCross::CENTER};
};

class ColumnWidget : public ContainerWidget {
 public:
  void set_gap(int gap) { this->gap_ = gap; }
  void set_main_align(AlignMain align) { this->main_align_ = align; }
  void set_cross_align(AlignCross align) { this->cross_align_ = align; }
  void layout(int origin_x, int origin_y, int avail_w, int avail_h) override;
  void draw(DrawContext &ctx, uint32_t now_ms) override;
  int intrinsic_width() const override;
  int intrinsic_height() const override;

 protected:
  int gap_{0};
  AlignMain main_align_{AlignMain::START};
  AlignCross cross_align_{AlignCross::START};
};

class BoxWidget : public Widget {
 public:
  void set_child(Widget *child) { this->child_ = child; }
  void set_fill(Color color) {
    this->fill_ = color;
    this->has_fill_ = true;
  }
  void set_padding(int padding) { this->padding_ = padding; }
  void set_shape(BoxShape shape) { this->shape_ = shape; }
  void set_point(BoxPoint point) { this->point_ = point; }
  void set_stroke(int stroke) { this->stroke_ = stroke; }
  void set_radius(int radius) { this->radius_ = radius; }
  void set_antialias(bool antialias) { this->antialias_ = antialias; }
  void layout(int origin_x, int origin_y, int avail_w, int avail_h) override;
  void draw(DrawContext &ctx, uint32_t now_ms) override;
  int intrinsic_width() const override;
  int intrinsic_height() const override;
  void bind(PixelLayout *host) override;
  bool prepare(uint32_t now_ms) override;
  uint32_t tick_period_ms() const override;

 protected:
  Widget *child_{nullptr};
  Color fill_{};
  bool has_fill_{false};
  int padding_{0};
  BoxShape shape_{BoxShape::RECT};
  BoxPoint point_{BoxPoint::UP};
  int stroke_{1};
  int radius_{0};
  bool antialias_{false};
};

class TextWidget : public Widget {
 public:
  void set_text(const std::string &text) { this->text_ = text; }
  void set_format(const std::string &format) {
    if (this->text_.empty())
      this->text_ = format;
  }
#ifdef USE_FONT
  void set_font(font::Font *font) { this->font_ = font; }
  void set_icon_font(font::Font *font) { this->icons_.set_font(font); }
#endif
#ifdef USE_TEXT_SENSOR
  void set_text_sensor(text_sensor::TextSensor *sensor) { this->text_sensor_ = sensor; }
#endif
#ifdef USE_SENSOR
  void set_sensor(sensor::Sensor *sensor) { this->sensor_ = sensor; }
#endif
  void set_color(Color color) { this->color_ = color; }
  void set_icon_color(Color color) { this->icons_.set_color(color); }
  void set_icon(const std::string &icon) { this->icons_.set_start(icon); }
  void set_icon_end(const std::string &icon) { this->icons_.set_end(icon); }
  void set_icon_align(IconAlign align) { this->icons_.set_align(align); }
  void set_icon_gap(int gap) { this->icons_.set_gap(gap); }
  void set_text_align(IconAlign align) { this->text_align_ = align; }
  void set_unit(const std::string &unit) { this->unit_ = unit; }
  void set_caption(const std::string &caption) { this->caption_ = caption; }
  void set_style(TextStyle style) { this->style_ = style; }
  void bind(PixelLayout *host) override;
  void draw(DrawContext &ctx, uint32_t now_ms) override;
  int intrinsic_width() const override;
  int intrinsic_height() const override;

 protected:
  const char *resolved_text_() const { return this->rendered_; }
  void render_();
  void on_sensor_state_(float value);
  void on_text_state_(const std::string &value);

  std::string text_;
  std::string unit_;
  std::string caption_;
  TextStyle style_{TextStyle::TEXT};
  Color color_{255, 255, 255};
  SideIcons icons_;
  IconAlign text_align_{IconAlign::MIDDLE};
  char rendered_[96]{""};
#ifdef USE_FONT
  font::Font *font_{nullptr};
#endif
#ifdef USE_TEXT_SENSOR
  text_sensor::TextSensor *text_sensor_{nullptr};
  std::string text_state_;
#endif
#ifdef USE_SENSOR
  sensor::Sensor *sensor_{nullptr};
#endif
};

class IconWidget : public TextWidget {
 public:
  void set_codepoint(const std::string &glyph) { this->set_text(glyph); }
};

class ClockWidget : public Widget {
 public:
  ClockWidget() { this->icons_.set_color(Color(0, 200, 255)); }
  void set_face(ClockFace face) { this->face_ = face; }
  void set_theme(ClockTheme theme) { this->theme_ = theme; }
  void set_size(ClockSize size) { this->size_ = size; }
#ifdef USE_TIME
  void set_time(time::RealTimeClock *time) { this->time_ = time; }
  void set_fallback_time(time::RealTimeClock *time) { this->fallback_time_ = time; }
#endif
#ifdef USE_FONT
  void set_font(font::Font *font) { this->font_ = font; }
  void set_icon_font(font::Font *font) { this->icons_.set_font(font); }
#endif
  void set_color(Color color) { this->color_ = color; }
  void set_icon_color(Color color) { this->icons_.set_color(color); }
  void set_secondary_color(Color color) {
    this->secondary_color_ = color;
    this->has_secondary_color_ = true;
  }
  void set_face_color(Color color) { this->face_color_ = color; }
  void set_tick_color(Color color) { this->tick_color_ = color; }
  void set_icon(const std::string &icon) { this->icons_.set_start(icon); }
  void set_icon_end(const std::string &icon) { this->icons_.set_end(icon); }
  void set_icon_align(IconAlign align) { this->icons_.set_align(align); }
  void set_icon_gap(int gap) { this->icons_.set_gap(gap); }
  void set_text_align(IconAlign align) { this->text_align_ = align; }
  void set_format(const std::string &format) { this->format_ = format; }
  void set_blink_colon(bool blink) { this->blink_colon_ = blink; }
  void set_show_colon(bool show) { this->show_colon_ = show; }
  void set_show_seconds(bool show) { this->show_seconds_ = show; }
  void set_ghost(bool ghost) { this->ghost_ = ghost; }
  bool prepare(uint32_t now_ms) override;
  void draw(DrawContext &ctx, uint32_t now_ms) override;
  int intrinsic_width() const override;
  int intrinsic_height() const override;
  uint32_t tick_period_ms() const override;

 protected:
  bool refresh_time_(uint32_t now_ms);
  bool arm_flaps_(uint32_t now_ms, bool old_valid, int old_hour, int old_minute, int old_second);
  bool flaps_active_(uint32_t now_ms) const;
  void draw_digital_(DrawContext &ctx, int x, int y, uint8_t alpha, Color color, uint32_t now_ms);
  void draw_analog_(DrawContext &ctx, int x, int y, uint8_t alpha, Color color);
  void paint_clock_(DrawContext &ctx, int x, int y, uint8_t alpha, Color color, uint32_t now_ms);

  ClockFace face_{ClockFace::DIGITAL};
  ClockTheme theme_{ClockTheme::SEVEN_SEGMENT};
  ClockSize size_{ClockSize::MD};
  Color color_{255, 255, 255};
  Color secondary_color_{0, 0, 0};
  bool has_secondary_color_{false};
  Color face_color_{40, 40, 40};
  Color tick_color_{80, 80, 80};
  SideIcons icons_;
  IconAlign text_align_{IconAlign::MIDDLE};
  std::string format_;
  bool blink_colon_{true};
  bool show_colon_{true};
  bool show_seconds_{false};
  bool ghost_{false};
  int hour_{0};
  int minute_{0};
  int second_{0};
  bool time_valid_{false};
  bool colon_on_{true};
  int8_t flap_from_[6]{-1, -1, -1, -1, -1, -1};
  int8_t flap_to_[6]{-1, -1, -1, -1, -1, -1};
  uint32_t flap_start_ms_[6]{0, 0, 0, 0, 0, 0};
  uint32_t flap_until_ms_[6]{0, 0, 0, 0, 0, 0};
  bool flap_on_[6]{false, false, false, false, false, false};
  char label_[32]{"--:--"};
#ifdef USE_TIME
  time::RealTimeClock *time_{nullptr};
  time::RealTimeClock *fallback_time_{nullptr};
#endif
#ifdef USE_FONT
  font::Font *font_{nullptr};
#endif
};

class DateWidget : public Widget {
 public:
  DateWidget() { this->icons_.set_color(Color(160, 160, 160)); }
  void set_style(DateStyle style) { this->style_ = style; }
#ifdef USE_TIME
  void set_time(time::RealTimeClock *time) { this->time_ = time; }
#endif
#ifdef USE_FONT
  void set_font(font::Font *font) { this->font_ = font; }
  void set_icon_font(font::Font *font) { this->icons_.set_font(font); }
#endif
  void set_color(Color color) { this->color_ = color; }
  void set_icon_color(Color color) { this->icons_.set_color(color); }
  void set_icon(const std::string &icon) { this->icons_.set_start(icon); }
  void set_icon_end(const std::string &icon) { this->icons_.set_end(icon); }
  void set_icon_align(IconAlign align) { this->icons_.set_align(align); }
  void set_icon_gap(int gap) { this->icons_.set_gap(gap); }
  void set_text_align(IconAlign align) { this->text_align_ = align; }
  void set_format(const std::string &format) { this->format_ = format; }
  void set_uppercase(bool uppercase) { this->uppercase_ = uppercase; }
  void set_show_year(bool show) { this->show_year_ = show; }
  bool prepare(uint32_t now_ms) override;
  void draw(DrawContext &ctx, uint32_t now_ms) override;
  int intrinsic_width() const override;
  int intrinsic_height() const override;
  uint32_t tick_period_ms() const override;

 protected:
  bool refresh_date_(uint32_t now_ms);

  DateStyle style_{DateStyle::TEXT};
  Color color_{160, 160, 160};
  SideIcons icons_;
  IconAlign text_align_{IconAlign::MIDDLE};
  std::string format_{"%a %d %b"};
  bool uppercase_{false};
  bool show_year_{false};
  bool time_valid_{false};
  int day_{0};
  int month_{0};
  int year_{0};
  char label_[40]{"--"};
  char weekday_[12]{"--"};
  char month_label_[8]{"--"};
#ifdef USE_TIME
  time::RealTimeClock *time_{nullptr};
#endif
#ifdef USE_FONT
  font::Font *font_{nullptr};
#endif
};

class WeatherWidget : public Widget {
 public:
#ifdef USE_TEXT_SENSOR
  void set_condition_sensor(text_sensor::TextSensor *sensor) { this->condition_sensor_ = sensor; }
#endif
#ifdef USE_SENSOR
  void set_temperature_sensor(sensor::Sensor *sensor) { this->temperature_sensor_ = sensor; }
  void set_humidity_sensor(sensor::Sensor *sensor) { this->humidity_sensor_ = sensor; }
  void set_wind_speed_sensor(sensor::Sensor *sensor) { this->wind_speed_sensor_ = sensor; }
  void set_wind_bearing_sensor(sensor::Sensor *sensor) { this->wind_bearing_sensor_ = sensor; }
#endif
#ifdef USE_FONT
  void set_font(font::Font *font) { this->font_ = font; }
  void set_icon_font(font::Font *font) { this->icon_font_ = font; }
#endif
  void set_color(Color color) { this->color_ = color; }
  void set_condition(const std::string &condition) { this->apply_condition_(condition.c_str()); }
  void set_show_icon(bool show) { this->show_icon_ = show; }
  void set_show_condition(bool show) { this->show_condition_ = show; }
  void set_show_temp(bool show) { this->show_temp_ = show; }
  void set_show_humidity(bool show) { this->show_humidity_ = show; }
  void set_show_wind(bool show) { this->show_wind_ = show; }
  void set_text_position(WeatherTextPosition pos) { this->text_position_ = pos; }
  void set_icon_align(IconAlign align) { this->icon_align_ = align; }
  void set_text_align(IconAlign align) { this->text_align_ = align; }
  void set_gap(int gap) { this->gap_ = gap < 0 ? 0 : gap > 16 ? 16 : gap; }
  void add_custom_icon(const std::string &key, const uint8_t *pixels, int width, int height, Color color,
                       const Color *palette, size_t palette_count);
  void bind(PixelLayout *host) override;
  void draw(DrawContext &ctx, uint32_t now_ms) override;
  int intrinsic_width() const override;
  int intrinsic_height() const override;

 protected:
  void apply_condition_(const char *condition);
  void format_temp_(char *out, size_t out_size) const;
  void format_humidity_(char *out, size_t out_size) const;
  void format_wind_(char *out, size_t out_size) const;
  int measure_line_(const char *text) const;
  int icon_size_() const;
  int line_height_() const;
  int text_ink_height_() const;
  int text_block_width_() const;
  int text_block_height_() const;
  const WeatherCustomIcon *find_custom_icon_(const char *key) const;
  void draw_custom_icon_(DrawContext &ctx, int x, int y, const WeatherCustomIcon *icon, uint8_t a, const Color *ink = nullptr) const;
  Color color_{0, 200, 255};
  std::string glyph_;
  std::string label_;
  char condition_key_[48]{};
  struct CustomIconEntry {
    std::string key;
    WeatherCustomIcon icon;
    std::vector<uint8_t> pixel_storage;
    std::vector<Color> palette_storage;
  };
  void rebind_custom_icon_ptrs_();
  std::vector<CustomIconEntry> custom_icons_;
  bool show_icon_{true};
  bool show_condition_{false};
  bool show_temp_{false};
  bool show_humidity_{false};
  bool show_wind_{false};
  WeatherTextPosition text_position_{WeatherTextPosition::END};
  IconAlign icon_align_{IconAlign::MIDDLE};
  IconAlign text_align_{IconAlign::MIDDLE};
  int gap_{2};
#ifdef USE_TEXT_SENSOR
  text_sensor::TextSensor *condition_sensor_{nullptr};
#endif
#ifdef USE_SENSOR
  sensor::Sensor *temperature_sensor_{nullptr};
  sensor::Sensor *humidity_sensor_{nullptr};
  sensor::Sensor *wind_speed_sensor_{nullptr};
  sensor::Sensor *wind_bearing_sensor_{nullptr};
#endif
#ifdef USE_FONT
  font::Font *font_{nullptr};
  font::Font *icon_font_{nullptr};
#endif
};

class SpriteWidget : public Widget {
 public:
#ifdef USE_IMAGE
  void set_image(image::Image *image) { this->image_ = image; }
#endif
#ifdef USE_ANIMATION
  void set_gif(animation::Animation *animation) { this->animation_ = animation; }
#endif
  void set_frame_width(int width) { this->frame_width_ = width; }
  void set_frame_height(int height) { this->frame_height_ = height; }
  void set_frames(int frames) { this->frames_ = frames; }
  void set_fps(float fps) { this->fps_ = fps; }
  void set_loop(bool loop) { this->loop_ = loop; }
  bool prepare(uint32_t now_ms) override;
  void draw(DrawContext &ctx, uint32_t now_ms) override;
  int intrinsic_width() const override;
  int intrinsic_height() const override;
  uint32_t tick_period_ms() const override;

 protected:
#ifdef USE_IMAGE
  image::Image *image_{nullptr};
#endif
#ifdef USE_ANIMATION
  animation::Animation *animation_{nullptr};
#endif
  int frame_width_{0};
  int frame_height_{0};
  int frames_{0};
  float fps_{10.0f};
  bool loop_{true};
  uint32_t last_frame_ms_{0};
  int current_frame_{0};
};

class CustomWidget : public Widget {
 public:
  void set_color(Color color) { this->color_ = color; }
  void add_palette_color(Color color) { this->palette_.push_back(color); }
  void set_pixels(const uint8_t *data, int width, int height) {
    this->pixels_ = data;
    this->pix_w_ = width;
    this->pix_h_ = height;
  }
  void draw(DrawContext &ctx, uint32_t now_ms) override;
  int intrinsic_width() const override;
  int intrinsic_height() const override;

 protected:
  uint8_t index_at_(int x, int y) const;
  Color color_for_index_(uint8_t idx) const;
  Color color_{255, 255, 255};
  std::vector<Color> palette_{};
  const uint8_t *pixels_{nullptr};
  int pix_w_{0};
  int pix_h_{0};
};

class PixelLayout : public Component {
 public:
  ~PixelLayout() { delete[] this->buffer_; }
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::PROCESSOR; }

  void set_display(display::Display *display) { this->display_ = display; }
  void set_root(Widget *root);
  void add_screen(Widget *root, uint32_t duration_ms);
  void add_screen(Widget *root, uint32_t duration_ms, ScreenTransition transition, uint32_t transition_ms);
  void add_screen(Widget *root, uint32_t duration_ms, ScreenTransition transition, uint32_t transition_ms,
                  const std::string &id);
  void set_rotate_ms(uint32_t ms);
  void set_transition(ScreenTransition type);
  void set_transition_ms(uint32_t ms);
  void set_screen_loop(bool loop) { this->screen_loop_ = loop; }
  void set_screen_random(bool random);
  bool get_screen_random() const { return this->screen_random_; }
  void set_background(Color color) { this->background_ = color; }

  /** Jump to screen by index or id (cut). Resets dwell; respects pin for auto-advance only. */
  bool show_screen(size_t index);
  bool show_screen(const std::string &id);
  /** Advance to next enabled screen (no-op if pinned or fewer than two enabled). */
  bool show_next_enabled();
  void set_pinned(bool pinned);
  bool is_pinned() const { return this->pinned_; }
  void set_screen_enabled(size_t index, bool enabled);
  bool is_screen_enabled(size_t index) const;
  size_t screen_count() const { return this->screens_.size(); }
  size_t current_screen_index() const { return this->screen_index_; }
  const std::string &screen_id(size_t index) const;
  std::string current_screen_id() const;

  void set_rotate_override(bool on);
  bool get_rotate_override() const { return this->rotate_override_; }
  uint32_t get_rotate_ms() const { return this->rotate_ms_; }
  void set_transition_override(bool on);
  bool get_transition_override() const { return this->transition_override_; }
  ScreenTransition get_transition() const { return this->transition_; }
  uint32_t get_transition_ms() const { return this->transition_ms_; }

  /** Optional nighttime blank schedule (compositor blank; Power stays on). */
  void set_night_schedule_configured(bool on) { this->night_configured_ = on; }
  bool night_schedule_configured() const { return this->night_configured_; }
#ifdef USE_TIME
  void set_night_schedule_time(time::RealTimeClock *rtc) { this->night_time_ = rtc; }
#endif
  void set_night_schedule_enabled(bool on);
  bool get_night_schedule_enabled() const { return this->night_enabled_; }
  void set_night_off_hour(uint8_t hour);
  void set_night_off_minute(uint8_t minute);
  void set_night_on_hour(uint8_t hour);
  void set_night_on_minute(uint8_t minute);
  uint8_t get_night_off_hour() const { return this->night_off_hour_; }
  uint8_t get_night_off_minute() const { return this->night_off_minute_; }
  uint8_t get_night_on_hour() const { return this->night_on_hour_; }
  uint8_t get_night_on_minute() const { return this->night_on_minute_; }
  /** Blank until next wake time. */
  void sleep_until_wake();
  /** Power ON override: clear sleep, show until next off edge. */
  void on_power_on();
  bool is_blanked() const { return this->blanked_; }
  /** Recompute blank state (host tests / after clock set). */
  void evaluate_night_schedule();

#ifdef USE_PIXEL_LAYOUT_SD_STORAGE
  void configure_sd_storage(const std::string &mount_path, const std::string &root_path, int clk_pin, int cmd_pin,
                            int d0_pin, uint16_t upload_port);
  void reload_from_sd();
  SdStorageManager &sd_storage() { return this->sd_storage_; }
#endif

  using PlaylistCallback = std::function<void()>;
  void add_on_playlist_change(PlaylistCallback cb) { this->playlist_cbs_.push_back(std::move(cb)); }

  void request_redraw();
  void invalidate_layout();
  void host_init(int width, int height);
  void host_shutdown();
  void host_set_play(size_t from, size_t to, bool transitioning, uint32_t trans_started_ms, ScreenTransition kind,
                     uint32_t trans_ms);
  void host_paint();
  /** Advance playlist once (host tests / preview). Honors pin, enables, overrides. */
  void host_advance(uint32_t now_ms) { this->advance_screens_(now_ms); }
  const uint8_t *host_buffer() const { return this->buffer_; }
  int host_width() const { return this->ctx_.width(); }
  int host_height() const { return this->ctx_.height(); }
#ifdef USE_FONT
  void set_font(font::Font *font) { this->font_ = font; }
  void set_icon_font(font::Font *font) { this->icon_font_ = font; }
  font::Font *font() const { return this->font_; }
  font::Font *icon_font() const { return this->icon_font_; }
#endif

 protected:
  void render_(display::Display &it);
  void tick_();
  void reschedule_();
  uint32_t tick_period_ms_() const;
  void advance_screens_(uint32_t now_ms);
  size_t next_screen_() const;
  size_t choose_next_screen_();
  bool screen_enabled_(size_t index) const;
  uint32_t dwell_ms_for_(size_t index) const;
  Widget *active_root_() const;
  void compose_(uint32_t now);
  ScreenTransition transition_for_(size_t index) const;
  uint32_t transition_ms_for_(size_t index) const;
  void notify_playlist_();
  void load_prefs_();
  void save_prefs_();
  void load_night_prefs_();
  void save_night_prefs_();
  void ensure_enabled_vectors_();
  bool in_night_window_() const;
  bool should_blank_() const;
  int night_minutes_now_() const;

  display::Display *display_{nullptr};
  Widget *root_{nullptr};
  std::vector<Widget *> screens_{};
  std::vector<uint32_t> screen_duration_ms_{};
  std::vector<ScreenTransition> screen_transition_{};
  std::vector<uint32_t> screen_transition_ms_{};
  std::vector<std::string> screen_ids_{};
  std::vector<uint8_t> screen_enabled_flags_{};
  uint32_t rotate_ms_{8000};
  uint32_t rotate_default_ms_{8000};
  uint32_t transition_ms_{400};
  uint32_t transition_default_ms_{400};
  ScreenTransition transition_{ScreenTransition::FADE};
  ScreenTransition transition_default_{ScreenTransition::FADE};
  ScreenTransition trans_play_{ScreenTransition::FADE};
  uint32_t trans_play_ms_{400};
  bool screen_loop_{true};
  bool screen_random_{false};
  bool pinned_{false};
  bool rotate_override_{false};
  bool transition_override_{false};
  bool night_configured_{false};
  bool night_enabled_{true};
  bool sleep_until_wake_{false};
  bool user_override_{false};
  bool blanked_{false};
  bool was_in_night_window_{false};
  uint8_t night_off_hour_{23};
  uint8_t night_off_minute_{0};
  uint8_t night_on_hour_{7};
  uint8_t night_on_minute_{0};
#ifdef USE_TIME
  time::RealTimeClock *night_time_{nullptr};
#endif
  std::vector<uint8_t> screen_seen_{};
  size_t screen_index_{0};
  size_t next_index_{0};
  uint32_t screen_started_ms_{0};
  bool transitioning_{false};
  uint32_t trans_started_ms_{0};
  Color background_{0, 0, 0};
  DrawContext ctx_{};
  uint8_t *buffer_{nullptr};
  bool dirty_{true};
  bool laid_out_{false};
  uint32_t period_ms_{500};
  std::vector<PlaylistCallback> playlist_cbs_{};
  bool prefs_loaded_{false};
#ifdef USE_FONT
  font::Font *font_{nullptr};
  font::Font *icon_font_{nullptr};
#endif
#ifdef USE_PIXEL_LAYOUT_SD_STORAGE
  SdStorageManager sd_storage_;
#endif
};

const char *screen_transition_name(ScreenTransition t);
bool screen_transition_from_name(const char *name, ScreenTransition *out);

}  // namespace pixel_layout
}  // namespace esphome
