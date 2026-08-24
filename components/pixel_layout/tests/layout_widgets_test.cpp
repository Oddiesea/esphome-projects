#include "pixel_layout.h"
#include "test_helpers.h"

#include "esphome/components/sensor/sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/time/real_time_clock.h"
#include "esphome/core/hal.h"

#include <gtest/gtest.h>

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <limits>
#include <vector>

using esphome::Color;
using esphome::pixel_layout::AlignMain;
using esphome::pixel_layout::ClockFace;
using esphome::pixel_layout::ClockSize;
using esphome::pixel_layout::ClockTheme;
using esphome::pixel_layout::ClockWidget;
using esphome::pixel_layout::ColumnWidget;
using esphome::pixel_layout::CustomWidget;
using esphome::pixel_layout::DrawContext;
using esphome::pixel_layout::RowWidget;
using esphome::pixel_layout::StackWidget;
using esphome::pixel_layout::Widget;
using esphome::pixel_layout::clock_metrics;
using esphome::pixel_layout::compass_point;
using esphome::pixel_layout::measure_digital_clock;
using esphome::pixel_layout::theme_metrics;
using esphome::pixel_layout::weather_glyph;
using esphome::pixel_layout::weather_key;
using esphome::pixel_layout::weather_label;
using esphome::pixel_layout::Outline;
using esphome::pixel_layout::WeatherWidget;
using esphome::pixel_layout::test::ProbeCustom;
using esphome::pixel_layout::test::ProbeWidget;
using esphome::pixel_layout::test::nonzero_count;

TEST(Metrics, ThemeDefaults) {
  EXPECT_EQ(theme_metrics(ClockTheme::SEVEN_SEGMENT).digit_w, 16);
  EXPECT_EQ(theme_metrics(ClockTheme::SEVEN_SEGMENT).digit_h, 28);
  EXPECT_FALSE(theme_metrics(ClockTheme::SEVEN_SEGMENT).rounded);
  EXPECT_TRUE(theme_metrics(ClockTheme::ROUNDED).rounded);
  EXPECT_TRUE(theme_metrics(ClockTheme::BLOCK).grid);
  EXPECT_TRUE(theme_metrics(ClockTheme::TINY).grid);
  EXPECT_EQ(theme_metrics(ClockTheme::TYPEFACE).digit_w, 16);
  EXPECT_EQ(theme_metrics(ClockTheme::SPLIT_FLAP).digit_w, 22);
  EXPECT_EQ(theme_metrics(ClockTheme::SPLIT_FLAP).digit_h, 36);
  EXPECT_TRUE(theme_metrics(ClockTheme::SPLIT_FLAP).grid);
  EXPECT_EQ(theme_metrics(ClockTheme::PERSPECTIVE).digit_w, 24);
  EXPECT_EQ(theme_metrics(ClockTheme::PERSPECTIVE).digit_h, 52);
  EXPECT_EQ(theme_metrics(ClockTheme::PERSPECTIVE).thickness_top, 7);
  EXPECT_EQ(theme_metrics(ClockTheme::PERSPECTIVE).thickness_mid, 4);
  EXPECT_EQ(theme_metrics(ClockTheme::PERSPECTIVE).thickness_bot, 2);
  EXPECT_EQ(theme_metrics(ClockTheme::PERSPECTIVE).taper, 10);
}

TEST(Metrics, ClockMetricsScaleAndUnscaled) {
  const auto seven_md = clock_metrics(ClockTheme::SEVEN_SEGMENT, ClockSize::MD);
  EXPECT_EQ(seven_md.digit_w, 16);
  const auto seven_sm = clock_metrics(ClockTheme::SEVEN_SEGMENT, ClockSize::SM);
  EXPECT_LT(seven_sm.digit_w, seven_md.digit_w);
  const auto seven_lg = clock_metrics(ClockTheme::SEVEN_SEGMENT, ClockSize::LG);
  EXPECT_GT(seven_lg.digit_w, seven_md.digit_w);

  EXPECT_EQ(clock_metrics(ClockTheme::TINY, ClockSize::SM).digit_w, theme_metrics(ClockTheme::TINY).digit_w);
  EXPECT_EQ(clock_metrics(ClockTheme::TINY, ClockSize::LG).digit_w, theme_metrics(ClockTheme::TINY).digit_w);
  EXPECT_EQ(clock_metrics(ClockTheme::TYPEFACE, ClockSize::LG).digit_w, theme_metrics(ClockTheme::TYPEFACE).digit_w);

  EXPECT_EQ(clock_metrics(ClockTheme::BLOCK, ClockSize::SM).cell, 1);
  EXPECT_EQ(clock_metrics(ClockTheme::BLOCK, ClockSize::MD).cell, 2);
  EXPECT_EQ(clock_metrics(ClockTheme::BLOCK, ClockSize::LG).cell, 3);
  EXPECT_EQ(clock_metrics(ClockTheme::BLOCK, ClockSize::SM).digit_w, 3);
  EXPECT_EQ(clock_metrics(ClockTheme::BLOCK, ClockSize::LG).digit_h, 15);

  EXPECT_EQ(clock_metrics(ClockTheme::SPLIT_FLAP, ClockSize::MD).digit_w, 22);
  EXPECT_LT(clock_metrics(ClockTheme::SPLIT_FLAP, ClockSize::SM).digit_w,
            clock_metrics(ClockTheme::SPLIT_FLAP, ClockSize::MD).digit_w);
  EXPECT_GT(clock_metrics(ClockTheme::SPLIT_FLAP, ClockSize::LG).digit_h,
            clock_metrics(ClockTheme::SPLIT_FLAP, ClockSize::MD).digit_h);

  EXPECT_EQ(clock_metrics(ClockTheme::PERSPECTIVE, ClockSize::MD).thickness_top, 7);
  EXPECT_LT(clock_metrics(ClockTheme::PERSPECTIVE, ClockSize::SM).digit_h,
            clock_metrics(ClockTheme::PERSPECTIVE, ClockSize::MD).digit_h);
  EXPECT_GT(clock_metrics(ClockTheme::PERSPECTIVE, ClockSize::LG).digit_h,
            clock_metrics(ClockTheme::PERSPECTIVE, ClockSize::MD).digit_h);
}

TEST(Metrics, MeasureDigitalClock) {
  int w = 0, h = 0;
  measure_digital_clock(ClockTheme::SEVEN_SEGMENT, ClockSize::MD, false, &w, &h);
  EXPECT_EQ(w, 4 * 16 + 6 + 4 * 2);
  EXPECT_EQ(h, 28);
  int w2 = 0, h2 = 0;
  measure_digital_clock(ClockTheme::SEVEN_SEGMENT, ClockSize::MD, true, &w2, &h2);
  EXPECT_GT(w2, w);
  EXPECT_EQ(h2, 28);
  int wf = 0, hf = 0;
  measure_digital_clock(ClockTheme::SPLIT_FLAP, ClockSize::MD, false, &wf, &hf);
  EXPECT_EQ(wf, 4 * 22 + 8 + 4 * 3);
  EXPECT_EQ(hf, 36);
}

TEST(Weather, GlyphAndLabel) {
  EXPECT_STREQ(weather_glyph(nullptr), weather_glyph("cloudy"));
  EXPECT_STREQ(weather_glyph(""), weather_glyph("cloudy"));
  EXPECT_STREQ(weather_glyph("sunny"), weather_glyph("clear"));
  EXPECT_STREQ(weather_glyph("mdi:weather-rainy"), weather_glyph("rain"));
  EXPECT_STREQ(weather_glyph("unknown-xyz"), weather_glyph("cloudy"));
  EXPECT_EQ(weather_label("sunny"), "Sunny");
  EXPECT_EQ(weather_label("partly-cloudy-day"), "Mixed");
  EXPECT_EQ(weather_label("mdi:weather-fog"), "Fog");
  EXPECT_EQ(weather_label("totally-made-up"), "Totally Made Up");
  EXPECT_EQ(weather_label(""), "Cloudy");
}

TEST(Weather, WeatherKey) {
  char key[32];
  weather_key("mdi:weather-rainy", key, sizeof(key));
  EXPECT_STREQ(key, "rainy");
  weather_key("partly-cloudy-day", key, sizeof(key));
  EXPECT_STREQ(key, "partly-cloudy-day");
  weather_key("", key, sizeof(key));
  EXPECT_STREQ(key, "cloudy");
  weather_key(nullptr, key, sizeof(key));
  EXPECT_STREQ(key, "cloudy");
}

TEST(Weather, EmptyConditionUsesCustomCloudy) {
  // 2x2: index 1 at (0,0) → widget/icon color
  const uint8_t packed[] = {0x10, 0x00};
  WeatherWidget w;
  w.set_show_icon(true);
  w.set_show_condition(false);
  w.set_show_temp(false);
  w.set_show_humidity(false);
  w.set_show_wind(false);
  w.add_custom_icon("cloudy", packed, 2, 2, Color(10, 20, 30), nullptr, 0);
  w.add_custom_icon("sunny", packed, 2, 2, Color(255, 128, 0), nullptr, 0);
  // No set_condition — bind path uses apply_condition_("").
  w.set_condition("");
  w.layout(0, 0, 8, 8);
  std::vector<uint8_t> buf(8 * 8 * 3, 0);
  DrawContext ctx;
  ctx.init(buf.data(), 8, 8);
  w.draw(ctx, 0);
  EXPECT_EQ(buf[0], 10);
  EXPECT_EQ(buf[1], 20);
  EXPECT_EQ(buf[2], 30);
}

TEST(Weather, ManyPaletteIconsSurviveRealloc) {
  const uint8_t packed[] = {0x12, 0x00};  // idx 1 then 2
  WeatherWidget w;
  w.set_show_icon(true);
  w.set_show_condition(false);
  const Color pal[] = {Color(0, 255, 0)};
  for (int i = 0; i < 24; i++) {
    char key[16];
    snprintf(key, sizeof(key), "icon%d", i);
    w.add_custom_icon(key, packed, 2, 2, Color(255, 0, 0), pal, 1);
  }
  w.add_custom_icon("cloudy", packed, 2, 2, Color(1, 2, 3), pal, 1);
  w.set_condition("cloudy");
  w.layout(0, 0, 8, 8);
  std::vector<uint8_t> buf(8 * 8 * 3, 0);
  DrawContext ctx;
  ctx.init(buf.data(), 8, 8);
  w.draw(ctx, 0);
  EXPECT_EQ(buf[0], 1);
  EXPECT_EQ(buf[1], 2);
  EXPECT_EQ(buf[2], 3);
  EXPECT_EQ(buf[3], 0);  // palette green at (1,0)
  EXPECT_EQ(buf[4], 255);
  EXPECT_EQ(buf[5], 0);
}

TEST(Weather, CustomIconDrawAndAlias) {
  const uint8_t packed[] = {0x10, 0x00};
  esphome::pixel_layout::WeatherWidget w;
  w.set_show_icon(true);
  w.set_show_condition(false);
  w.set_show_temp(false);
  w.set_show_humidity(false);
  w.set_show_wind(false);
  w.add_custom_icon("sunny", packed, 2, 2, Color(255, 128, 0), nullptr, 0);
  w.add_custom_icon("default", packed, 2, 2, Color(100, 100, 100), nullptr, 0);
  w.set_condition("clear");
  w.layout(0, 0, 8, 8);
  std::vector<uint8_t> buf(8 * 8 * 3, 0);
  DrawContext ctx;
  ctx.init(buf.data(), 8, 8);
  w.draw(ctx, 0);
  EXPECT_EQ(buf[0], 255);
  EXPECT_EQ(buf[1], 128);

  w.set_condition("unknown-xyz");
  buf.assign(8 * 8 * 3, 0);
  ctx.init(buf.data(), 8, 8);
  w.draw(ctx, 0);
  EXPECT_EQ(buf[0], 100);
  EXPECT_EQ(w.intrinsic_width(), 2);
}

TEST(Weather, CustomIconOutline) {
  const uint8_t packed[] = {0x10, 0x00};
  WeatherWidget w;
  w.set_show_icon(true);
  w.set_show_condition(false);
  w.set_outline(Outline::BLACK);
  w.add_custom_icon("sunny", packed, 2, 2, Color(255, 128, 0), nullptr, 0);
  w.set_condition("sunny");
  w.layout(0, 0, 8, 8);
  std::vector<uint8_t> buf(8 * 8 * 3, 0);
  DrawContext ctx;
  ctx.init(buf.data(), 8, 8);
  w.draw(ctx, 0);
  EXPECT_EQ(buf[0], 255);
  EXPECT_EQ(buf[1], 128);
  EXPECT_EQ(buf[3], 0);
  EXPECT_EQ(buf[(1 * 8 + 0) * 3], 0);
}

TEST(Weather, CompassPoints) {
  EXPECT_STREQ(compass_point(0), "N");
  EXPECT_STREQ(compass_point(45), "NE");
  EXPECT_STREQ(compass_point(90), "E");
  EXPECT_STREQ(compass_point(180), "S");
  EXPECT_STREQ(compass_point(270), "W");
  EXPECT_STREQ(compass_point(360), "N");
  EXPECT_STREQ(compass_point(-45), "NW");
  EXPECT_STREQ(compass_point(std::numeric_limits<float>::quiet_NaN()), "");
}

TEST(CustomWidget, NibblePackingAndPalette) {
  // 2x2: 1,2 / 0,3 packed as high-nibble first
  const uint8_t packed[] = {0x12, 0x03};
  ProbeCustom w;
  w.set_color(Color(255, 0, 0));
  w.add_palette_color(Color(0, 255, 0));
  w.add_palette_color(Color(0, 0, 255));
  w.set_pixels(packed, 2, 2);
  EXPECT_EQ(w.index_at(0, 0), 1);
  EXPECT_EQ(w.index_at(1, 0), 2);
  EXPECT_EQ(w.index_at(0, 1), 0);
  EXPECT_EQ(w.index_at(1, 1), 3);
  EXPECT_EQ(w.index_at(-1, 0), 0);
  EXPECT_EQ(w.color_for_index(1).r, 255);
  EXPECT_EQ(w.color_for_index(2).g, 255);
  EXPECT_EQ(w.color_for_index(3).b, 255);
  EXPECT_EQ(w.intrinsic_width(), 2);
  EXPECT_EQ(w.intrinsic_height(), 2);
}

TEST(CustomWidget, DrawSkipsTransparent) {
  const uint8_t packed[] = {0x10, 0x02};
  CustomWidget w;
  w.set_color(Color(255, 0, 0));
  w.add_palette_color(Color(0, 255, 0));
  w.set_pixels(packed, 2, 2);
  w.layout(0, 0, 8, 8);
  std::vector<uint8_t> buf(8 * 8 * 3, 0);
  DrawContext ctx;
  ctx.init(buf.data(), 8, 8);
  w.draw(ctx, 0);
  EXPECT_EQ(buf[0], 255);
  EXPECT_EQ(buf[3], 0);
  EXPECT_EQ(buf[(1 * 8 + 0) * 3], 0);
  EXPECT_EQ(buf[(1 * 8 + 1) * 3 + 1], 255);
}

TEST(Layout, ExplicitVsIntrinsicVsAvail) {
  ProbeWidget w;
  w.set_width(10);
  w.set_height(6);
  w.layout(2, 3, 40, 20);
  EXPECT_EQ(w.box_x(), 2);
  EXPECT_EQ(w.box_y(), 3);
  EXPECT_EQ(w.box_w(), 10);
  EXPECT_EQ(w.box_h(), 6);

  ProbeWidget avail;
  avail.layout(0, 0, 32, 16);
  EXPECT_EQ(avail.box_w(), 32);
  EXPECT_EQ(avail.box_h(), 16);
}

TEST(Layout, StackUsesChildOffsets) {
  ProbeWidget a;
  a.set_width(8);
  a.set_height(4);
  a.set_x(2);
  a.set_y(1);
  ProbeWidget b;
  b.set_width(5);
  b.set_height(10);
  b.set_x(0);
  b.set_y(0);
  StackWidget stack;
  stack.add_child(&a);
  stack.add_child(&b);
  EXPECT_EQ(stack.intrinsic_width(), 10);
  EXPECT_EQ(stack.intrinsic_height(), 10);
  stack.layout(1, 2, 20, 20);
  EXPECT_EQ(a.box_x(), 1 + 2);
  EXPECT_EQ(a.box_y(), 2 + 1);
}

TEST(Layout, RowGapHiddenExpandedAndAlign) {
  ProbeWidget a;
  a.set_width(4);
  a.set_height(2);
  ProbeWidget b;
  b.set_width(4);
  b.set_height(2);
  ProbeWidget hidden;
  hidden.set_width(9);
  hidden.set_height(2);
  esphome::sensor::Sensor sensor;
  hidden.add_visible_sensor(&sensor, 1, 0, 0, 0, false);
  hidden.bind(nullptr);

  RowWidget row;
  row.set_gap(2);
  row.add_child(&a);
  row.add_child(&hidden);
  row.add_child(&b);
  EXPECT_FALSE(hidden.is_shown());
  EXPECT_EQ(row.intrinsic_width(), 4 + 2 + 4);
  EXPECT_EQ(row.intrinsic_height(), 2);

  row.set_width(20);
  row.set_height(8);
  row.set_main_align(AlignMain::END);
  row.layout(0, 0, 20, 8);
  EXPECT_EQ(b.box_x() + b.box_w(), 20);

  ProbeWidget flex;
  flex.set_expanded(true);
  RowWidget flex_row;
  flex_row.set_width(20);
  flex_row.set_height(4);
  flex_row.add_child(&a);
  flex_row.add_child(&flex);
  flex_row.layout(0, 0, 20, 4);
  EXPECT_GT(flex.box_w(), a.box_w());

  ProbeWidget c;
  c.set_width(2);
  c.set_height(2);
  ProbeWidget d;
  d.set_width(2);
  d.set_height(2);
  RowWidget spaced;
  spaced.set_width(20);
  spaced.set_height(4);
  spaced.set_main_align(AlignMain::SPACE_BETWEEN);
  spaced.add_child(&c);
  spaced.add_child(&d);
  spaced.layout(0, 0, 20, 4);
  EXPECT_EQ(c.box_x(), 0);
  EXPECT_EQ(d.box_x(), 18);
}

TEST(Layout, ColumnGapAndCenter) {
  ProbeWidget a;
  a.set_width(4);
  a.set_height(3);
  ProbeWidget b;
  b.set_width(6);
  b.set_height(3);
  ColumnWidget col;
  col.set_gap(1);
  col.add_child(&a);
  col.add_child(&b);
  EXPECT_EQ(col.intrinsic_width(), 6);
  EXPECT_EQ(col.intrinsic_height(), 7);
  col.set_width(10);
  col.set_height(10);
  col.layout(0, 0, 10, 10);
  EXPECT_EQ(a.box_y(), 0);
  EXPECT_EQ(b.box_y(), 4);
}

TEST(Visibility, SensorComparisonsAndInvert) {
  esphome::sensor::Sensor sensor;
  ProbeWidget w;
  w.add_visible_sensor(&sensor, 1, 10, 0, 0, false);
  w.bind(nullptr);
  EXPECT_FALSE(w.is_shown());
  sensor.set_state(11);
  EXPECT_TRUE(w.is_shown());
  sensor.set_state(10);
  EXPECT_FALSE(w.is_shown());

  ProbeWidget gte;
  gte.add_visible_sensor(&sensor, 2, 10, 0, 0, false);
  gte.bind(nullptr);
  sensor.set_state(10);
  EXPECT_TRUE(gte.is_shown());

  ProbeWidget eq;
  eq.add_visible_sensor(&sensor, 3, 10, 0, 0, false);
  eq.bind(nullptr);
  sensor.set_state(10);
  EXPECT_TRUE(eq.is_shown());
  sensor.set_state(10.5f);
  EXPECT_FALSE(eq.is_shown());

  ProbeWidget ne;
  ne.add_visible_sensor(&sensor, 4, 10, 0, 0, false);
  ne.bind(nullptr);
  sensor.set_state(9);
  EXPECT_TRUE(ne.is_shown());

  ProbeWidget lt;
  lt.add_visible_sensor(&sensor, 5, 10, 0, 0, false);
  lt.bind(nullptr);
  sensor.set_state(9);
  EXPECT_TRUE(lt.is_shown());

  ProbeWidget lte;
  lte.add_visible_sensor(&sensor, 6, 10, 0, 0, false);
  lte.bind(nullptr);
  sensor.set_state(10);
  EXPECT_TRUE(lte.is_shown());

  ProbeWidget inverted;
  inverted.add_visible_sensor(&sensor, 1, 10, 0, 0, true);
  inverted.bind(nullptr);
  sensor.set_state(11);
  EXPECT_FALSE(inverted.is_shown());

  sensor.clear_state();
  ProbeWidget nan_hidden;
  nan_hidden.add_visible_sensor(&sensor, 1, 0, 0, 0, false);
  nan_hidden.bind(nullptr);
  EXPECT_FALSE(nan_hidden.is_shown());
}

TEST(Visibility, TextAndOr) {
  esphome::text_sensor::TextSensor text;
  ProbeWidget w;
  w.add_visible_text(&text, "on", false);
  w.bind(nullptr);
  EXPECT_FALSE(w.is_shown());
  text.set_state("on");
  EXPECT_TRUE(w.is_shown());
  text.set_state("off");
  EXPECT_FALSE(w.is_shown());

  esphome::sensor::Sensor a;
  esphome::sensor::Sensor b;
  ProbeWidget either;
  either.set_visible_match_all(false);
  either.add_visible_sensor(&a, 1, 0, 0, 0, false);
  either.add_visible_sensor(&b, 1, 0, 0, 0, false);
  either.bind(nullptr);
  EXPECT_FALSE(either.is_shown());
  b.set_state(1);
  EXPECT_TRUE(either.is_shown());

  ProbeWidget both;
  both.set_visible_match_all(true);
  both.add_visible_sensor(&a, 1, 0, 0, 0, false);
  both.add_visible_sensor(&b, 1, 0, 0, 0, false);
  a.clear_state();
  b.set_state(1);
  both.bind(nullptr);
  EXPECT_FALSE(both.is_shown());
  a.set_state(1);
  EXPECT_TRUE(both.is_shown());
}

TEST(Animation, FadeDelayAndFinish) {
  ProbeWidget fade;
  fade.set_animation(esphome::pixel_layout::AnimType::FADE, 1000, 200, 0, 0, 0, 0, 255);
  fade.start_anim(1000);
  EXPECT_EQ(fade.opacity_at(1100, 255), 0);
  EXPECT_NEAR(fade.amount_at(1700), 0.5f, 0.02f);
  EXPECT_EQ(fade.opacity_at(1700, 255), 127);
  EXPECT_FLOAT_EQ(fade.amount_at(2300), 1.0f);
  EXPECT_EQ(fade.opacity_at(2300, 255), 255);
}

TEST(Animation, SlideOffset) {
  ProbeWidget slide;
  slide.set_animation(esphome::pixel_layout::AnimType::SLIDE, 100, 0, 0, 10, 4, 0, 255);
  slide.start_anim(0);
  int dx = 0, dy = 0;
  slide.offset_at(0, &dx, &dy);
  EXPECT_EQ(dx, 10);
  EXPECT_EQ(dy, 4);
  slide.offset_at(150, &dx, &dy);
  EXPECT_EQ(dx, 0);
  EXPECT_EQ(dy, 0);
}

TEST(Animation, BlinkAndPulseExtrema) {
  ProbeWidget blink;
  blink.set_animation(esphome::pixel_layout::AnimType::BLINK, 100, 0, -1, 0, 0, 0, 255);
  blink.start_anim(0);
  EXPECT_EQ(blink.opacity_at(10, 255), 255);
  EXPECT_EQ(blink.opacity_at(60, 255), 0);

  ProbeWidget pulse;
  pulse.set_animation(esphome::pixel_layout::AnimType::PULSE, 1000, 0, -1, 0, 0, 0, 255);
  pulse.start_anim(0);
  const uint8_t mid = pulse.opacity_at(0, 255);
  const uint8_t quarter = pulse.opacity_at(250, 255);
  EXPECT_NE(mid, quarter);
}

TEST(ClockWidget, InvalidAndValidDigital) {
  std::vector<uint8_t> buf(80 * 32 * 3, 0);
  DrawContext ctx;
  ctx.init(buf.data(), 80, 32);
  ClockWidget clock;
  clock.set_face(ClockFace::DIGITAL);
  clock.set_theme(ClockTheme::SEVEN_SEGMENT);
  clock.set_blink_colon(false);
  clock.set_width(78);
  clock.set_height(28);
  clock.layout(0, 0, 80, 32);
  clock.prepare(0);
  clock.draw(ctx, 0);
  const int dashes = nonzero_count(buf.data(), 80, 32);
  EXPECT_GT(dashes, 0);

  esphome::time::RealTimeClock rtc;
  esphome::time::ESPTime now;
  now.valid = true;
  now.hour = 12;
  now.minute = 34;
  now.second = 56;
  rtc.set_now(now);
  clock.set_time(&rtc);
  std::fill(buf.begin(), buf.end(), 0);
  clock.prepare(0);
  clock.draw(ctx, 0);
  EXPECT_GT(nonzero_count(buf.data(), 80, 32), dashes);
}

TEST(ClockWidget, GhostPaintsInactiveSegments) {
  std::vector<uint8_t> buf(80 * 32 * 3, 0);
  DrawContext ctx;
  ctx.init(buf.data(), 80, 32);
  ClockWidget clock;
  clock.set_face(ClockFace::DIGITAL);
  clock.set_theme(ClockTheme::SEVEN_SEGMENT);
  clock.set_blink_colon(false);
  clock.set_width(78);
  clock.set_height(28);
  esphome::time::RealTimeClock rtc;
  esphome::time::ESPTime now;
  now.valid = true;
  now.hour = 1;
  now.minute = 11;
  now.second = 11;
  rtc.set_now(now);
  clock.set_time(&rtc);
  clock.layout(0, 0, 80, 32);
  clock.prepare(0);
  clock.draw(ctx, 0);
  const int sharp = nonzero_count(buf.data(), 80, 32);
  std::fill(buf.begin(), buf.end(), 0);
  clock.set_ghost(true);
  clock.draw(ctx, 0);
  EXPECT_GT(nonzero_count(buf.data(), 80, 32), sharp);
}

TEST(ClockWidget, PerspectivePaintsTaperedDigits) {
  std::vector<uint8_t> buf(128 * 64 * 3, 0);
  DrawContext ctx;
  ctx.init(buf.data(), 128, 64);
  ClockWidget clock;
  clock.set_face(ClockFace::DIGITAL);
  clock.set_theme(ClockTheme::PERSPECTIVE);
  clock.set_size(ClockSize::MD);
  clock.set_blink_colon(false);
  clock.set_width(105);
  clock.set_height(52);
  esphome::time::RealTimeClock rtc;
  esphome::time::ESPTime now;
  now.valid = true;
  now.hour = 8;
  now.minute = 8;
  rtc.set_now(now);
  clock.set_time(&rtc);
  clock.layout(11, 11, 128, 64);
  clock.prepare(0);
  clock.draw(ctx, 0);
  EXPECT_GT(nonzero_count(buf.data(), 128, 64), 200);
}

TEST(ClockWidget, AnalogPaintsNearCenter) {
  std::vector<uint8_t> buf(40 * 40 * 3, 0);
  DrawContext ctx;
  ctx.init(buf.data(), 40, 40);
  ClockWidget clock;
  clock.set_face(ClockFace::ANALOG);
  clock.set_theme(ClockTheme::RING);
  clock.set_width(40);
  clock.set_height(40);
  esphome::time::RealTimeClock rtc;
  esphome::time::ESPTime now;
  now.valid = true;
  now.hour = 3;
  now.minute = 0;
  rtc.set_now(now);
  clock.set_time(&rtc);
  clock.layout(0, 0, 40, 40);
  clock.prepare(0);
  clock.draw(ctx, 0);
  EXPECT_GT(nonzero_count(buf.data(), 40, 40), 10);
  EXPECT_NE(buf[(20 * 40 + 20) * 3], 0);
}

TEST(ClockWidget, AnalogMinimalPaintsCardinals) {
  std::vector<uint8_t> buf(40 * 40 * 3, 0);
  DrawContext ctx;
  ctx.init(buf.data(), 40, 40);
  ClockWidget clock;
  clock.set_face(ClockFace::ANALOG);
  clock.set_theme(ClockTheme::MINIMAL);
  clock.set_width(40);
  clock.set_height(40);
  esphome::time::RealTimeClock rtc;
  esphome::time::ESPTime now;
  now.valid = true;
  now.hour = 3;
  now.minute = 0;
  rtc.set_now(now);
  clock.set_time(&rtc);
  clock.layout(0, 0, 40, 40);
  clock.prepare(0);
  clock.draw(ctx, 0);
  EXPECT_GT(nonzero_count(buf.data(), 40, 40), 8);
  EXPECT_NE(buf[(20 * 40 + 20) * 3], 0);
  EXPECT_NE(buf[(2 * 40 + 20) * 3], 0);
  EXPECT_NE(buf[(20 * 40 + 38) * 3], 0);
}

TEST(ClockWidget, AnalogTicksAndSquarePaint) {
  esphome::time::RealTimeClock rtc;
  esphome::time::ESPTime now;
  now.valid = true;
  now.hour = 10;
  now.minute = 10;
  rtc.set_now(now);

  auto paint = [&](ClockTheme theme) {
    std::vector<uint8_t> buf(40 * 40 * 3, 0);
    DrawContext ctx;
    ctx.init(buf.data(), 40, 40);
    ClockWidget clock;
    clock.set_face(ClockFace::ANALOG);
    clock.set_theme(theme);
    clock.set_width(40);
    clock.set_height(40);
    clock.set_time(&rtc);
    clock.layout(0, 0, 40, 40);
    clock.prepare(0);
    clock.draw(ctx, 0);
    return nonzero_count(buf.data(), 40, 40);
  };

  EXPECT_GT(paint(ClockTheme::TICKS), 12);
  EXPECT_GT(paint(ClockTheme::SQUARE), 20);
}

TEST(ClockWidget, SplitFlapFlipsWhenMinuteChanges) {
  std::vector<uint8_t> buf(128 * 64 * 3, 0);
  DrawContext ctx;
  ctx.init(buf.data(), 128, 64);
  ClockWidget clock;
  clock.set_face(ClockFace::DIGITAL);
  clock.set_theme(ClockTheme::SPLIT_FLAP);
  clock.set_blink_colon(false);
  clock.set_width(111);
  clock.set_height(36);
  esphome::time::RealTimeClock rtc;
  esphome::time::ESPTime now;
  now.valid = true;
  now.hour = 12;
  now.minute = 34;
  now.second = 0;
  rtc.set_now(now);
  clock.set_time(&rtc);
  clock.layout(0, 0, 128, 64);
  esphome::set_millis(1000);
  EXPECT_TRUE(clock.prepare(1000));
  clock.draw(ctx, 1000);
  const int first = nonzero_count(buf.data(), 128, 64);
  EXPECT_GT(first, 50);

  now.minute = 35;
  rtc.set_now(now);
  EXPECT_TRUE(clock.prepare(1000));
  EXPECT_LE(clock.tick_period_ms(), 33u);
  EXPECT_TRUE(clock.prepare(1100));
  std::fill(buf.begin(), buf.end(), 0);
  clock.draw(ctx, 1100);
  EXPECT_GT(nonzero_count(buf.data(), 128, 64), 50);

  EXPECT_TRUE(clock.prepare(1100));
  esphome::set_millis(2000);
  EXPECT_FALSE(clock.prepare(2000));
}
